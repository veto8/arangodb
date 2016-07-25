////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2016 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Michael Hackstein
////////////////////////////////////////////////////////////////////////////////

#include "SingleServerTraverser.h"
#include "Utils/Transaction.h"
#include "VocBase/MasterPointer.h"

using namespace arangodb;
using namespace arangodb::traverser;

////////////////////////////////////////////////////////////////////////////////
/// @brief Get a document by it's ID. Also lazy locks the collection.
///        If DOCUMENT_NOT_FOUND this function will return normally
///        with a OperationResult.failed() == true.
///        On all other cases this function throws.
////////////////////////////////////////////////////////////////////////////////

static int FetchDocumentById(arangodb::Transaction* trx,
                             std::string const& id,
                             TRI_doc_mptr_t* mptr) {
  size_t pos = id.find('/');
  if (pos == std::string::npos) {
    TRI_ASSERT(false);
    return TRI_ERROR_INTERNAL;
  }
  if (id.find('/', pos + 1) != std::string::npos) {
    TRI_ASSERT(false);
    return TRI_ERROR_INTERNAL;
  }

  int res = trx->documentFastPathLocal(id.substr(0, pos), id.substr(pos + 1), mptr);

  if (res != TRI_ERROR_NO_ERROR && res != TRI_ERROR_ARANGO_DOCUMENT_NOT_FOUND) {
    THROW_ARANGO_EXCEPTION(res);
  }
  return res;
}

SingleServerEdgeCursor::SingleServerEdgeCursor(size_t nrCursors)
    : _cursors(), _currentCursor(0), _cachePos(0) {
  _cursors.reserve(nrCursors);
  _cache.reserve(1000);
};

bool SingleServerEdgeCursor::next(std::vector<VPackSlice>& result, size_t& cursorId) {
  if (_currentCursor == _cursors.size()) {
    return false;
  }
  _cachePos++;
  if (_cachePos < _cache.size()) {
    result.emplace_back(_cache[_cachePos]->vpack());
    cursorId = _currentCursor;
    return true;
  }
  // We need to refill the cache.
  _cachePos = 0;
  auto& cursor = _cursors[_currentCursor];
  // NOTE: We cannot clear the cache,
  // because the cursor expect's it to be filled.
  do {
    if (!cursor->hasMore()) {
      // This one is exhausted, next
      ++_currentCursor;
      if (_currentCursor == _cursors.size()) {
        // We are done, all cursors exhausted.
        return false;
      }
      cursor = _cursors[_currentCursor];
      // If we switch the cursor. We have to clear the cache.
      _cache.clear();
    } else {
      cursor->getMoreMptr(_cache);
    }
  } while (_cache.empty());
  TRI_ASSERT(_cachePos < _cache.size());
  result.emplace_back(_cache[_cachePos]->vpack());
  cursorId = _currentCursor;
  return true;
}

bool SingleServerEdgeCursor::readAll(std::unordered_set<VPackSlice>& result, size_t& cursorId) {
  if (_currentCursor >= _cursors.size()) {
    return false;
  }
  cursorId = _currentCursor;
  auto& cursor = _cursors[_currentCursor];
  while (cursor->hasMore()) {
    // NOTE: We cannot clear the cache,
    // because the cursor expect's it to be filled.
    cursor->getMoreMptr(_cache);
    for (auto const& mptr : _cache) {
      result.emplace(mptr->vpack());
    }
  }
  _currentCursor++;
  return true;
}

SingleServerTraverser::SingleServerTraverser(TraverserOptions* opts,
                                             arangodb::Transaction* trx)
    : Traverser(opts), _trx(trx), _startIdBuilder(trx) {
  if (opts->uniqueVertices == TraverserOptions::UniquenessLevel::GLOBAL) {
    _vertexGetter = std::make_unique<UniqueVertexGetter>(this);
  } else {
    _vertexGetter = std::make_unique<VertexGetter>(this);
  }
}

SingleServerTraverser::~SingleServerTraverser() {}

aql::AqlValue SingleServerTraverser::fetchVertexData(VPackSlice id) {
  TRI_ASSERT(id.isString());
  auto it = _vertices.find(id);

  if (it == _vertices.end()) {
    TRI_doc_mptr_t mptr;
#warning Do we need the copy here?
    int res = FetchDocumentById(_trx, id.copyString(), &mptr);
    ++_readDocuments;
    if (res != TRI_ERROR_NO_ERROR) {
      return aql::AqlValue(basics::VelocyPackHelper::NullValue());
    }

    uint8_t const* p = mptr.vpack();
    _vertices.emplace(id, p);
    return aql::AqlValue(p, aql::AqlValueFromMasterPointer());
  }

  return aql::AqlValue((*it).second, aql::AqlValueFromMasterPointer());
}

aql::AqlValue SingleServerTraverser::fetchEdgeData(VPackSlice edge) {
  return aql::AqlValue(edge);
}

void SingleServerTraverser::addVertexToVelocyPack(VPackSlice id,
                                                  VPackBuilder& result) {
  TRI_ASSERT(id.isString());
  auto it = _vertices.find(id);

  if (it == _vertices.end()) {
    TRI_doc_mptr_t mptr;
#warning Do we need the copy here?
    int res = FetchDocumentById(_trx, id.copyString(), &mptr);
    ++_readDocuments;
    if (res != TRI_ERROR_NO_ERROR) {
      result.add(basics::VelocyPackHelper::NullValue());
    } else {
      uint8_t const* p = mptr.vpack();
      _vertices.emplace(id, p);
      result.addExternal(p);
    }
  } else {
    result.addExternal((*it).second);
  }
}

void SingleServerTraverser::addEdgeToVelocyPack(VPackSlice edge,
    VPackBuilder& result) {
  result.addExternal(edge.begin());
}

bool SingleServerTraverser::VertexGetter::getVertex(
    VPackSlice edge, std::vector<VPackSlice>& result) {
  VPackSlice cmp = result.back();
  VPackSlice res = Transaction::extractFromFromDocument(edge);
  if (arangodb::basics::VelocyPackHelper::compare(cmp, res, false) == 0) {
    res = Transaction::extractToFromDocument(edge);
  }

  if (!_traverser->vertexMatchesConditions(res, result.size())) {
    return false;
  }
  result.emplace_back(res);
  return true;
}

bool SingleServerTraverser::VertexGetter::getSingleVertex(VPackSlice edge,
                                                          VPackSlice cmp,
                                                          size_t depth,
                                                          VPackSlice& result) {
  VPackSlice from = Transaction::extractFromFromDocument(edge);
  if (arangodb::basics::VelocyPackHelper::compare(cmp, from, false) != 0) {
    result = from;
  } else {
    result = Transaction::extractToFromDocument(edge);
  }
  return _traverser->vertexMatchesConditions(result, depth);
}



void SingleServerTraverser::VertexGetter::reset(arangodb::velocypack::Slice) {
}

bool SingleServerTraverser::UniqueVertexGetter::getVertex(
  VPackSlice edge, std::vector<VPackSlice>& result) {
  VPackSlice toAdd = Transaction::extractFromFromDocument(edge);
  VPackSlice cmp = result.back();

  if (arangodb::basics::VelocyPackHelper::compare(toAdd, cmp, false) == 0) {
    toAdd = Transaction::extractToFromDocument(edge);
  }
  

  // First check if we visited it. If not, than mark
  if (_returnedVertices.find(toAdd) != _returnedVertices.end()) {
    // This vertex is not unique.
    ++_traverser->_filteredPaths;
    return false;
  } else {
    _returnedVertices.emplace(toAdd);
  }

  if (!_traverser->vertexMatchesConditions(toAdd, result.size())) {
    return false;
  }

  result.emplace_back(toAdd);
  return true;
}

bool SingleServerTraverser::UniqueVertexGetter::getSingleVertex(
  VPackSlice edge, VPackSlice cmp, size_t depth, VPackSlice& result) {
  result = Transaction::extractFromFromDocument(edge);

  if (arangodb::basics::VelocyPackHelper::compare(result, cmp, false) == 0) {
    result = Transaction::extractToFromDocument(edge);
  }
  
  // First check if we visited it. If not, than mark
  if (_returnedVertices.find(result) != _returnedVertices.end()) {
    // This vertex is not unique.
    ++_traverser->_filteredPaths;
    return false;
  } else {
    _returnedVertices.emplace(result);
  }

  return _traverser->vertexMatchesConditions(result, depth);
}

void SingleServerTraverser::UniqueVertexGetter::reset(VPackSlice startVertex) {
  _returnedVertices.clear();
  // The startVertex always counts as visited!
  _returnedVertices.emplace(startVertex);
}

void SingleServerTraverser::setStartVertex(std::string const& v) {
  _pruneNext = false;

  _startIdBuilder->clear();
  _startIdBuilder->add(VPackValue(v));
  VPackSlice idSlice = _startIdBuilder->slice();

  if (!vertexMatchesConditions(idSlice, 0)) {
    // Start vertex invalid
    _done = true;
    return;
  }

  _vertexGetter->reset(idSlice);

  if (_opts->useBreadthFirst) {
    _enumerator.reset(new BreadthFirstEnumerator(this, idSlice, _opts));
  } else {
    _enumerator.reset(new DepthFirstEnumerator(this, idSlice, _opts));
  }
  _done = false;
}

bool SingleServerTraverser::getVertex(VPackSlice edge,
                                      std::vector<VPackSlice>& result) {
  return _vertexGetter->getVertex(edge, result);
}

bool SingleServerTraverser::getSingleVertex(VPackSlice edge, VPackSlice vertex,
                                            size_t depth, VPackSlice& result) {
  return _vertexGetter->getSingleVertex(edge, vertex, depth, result);
}

bool SingleServerTraverser::next() {
  TRI_ASSERT(!_done);
  bool res = _enumerator->next();
  if (!res) {
    _done = true;
  }
  return res;
}

aql::AqlValue SingleServerTraverser::lastVertexToAqlValue() {
  return _enumerator->lastVertexToAqlValue();
}

aql::AqlValue SingleServerTraverser::lastEdgeToAqlValue() {
  return _enumerator->lastEdgeToAqlValue();
}

aql::AqlValue SingleServerTraverser::pathToAqlValue(VPackBuilder& builder) {
  return _enumerator->pathToAqlValue(builder);
}

////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2024 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Business Source License 1.1 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     https://github.com/arangodb/arangodb/blob/devel/LICENSE
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdlib>
#include <string>

namespace arangodb {
class UniformCharacter {
 private:
  UniformCharacter(UniformCharacter const&);
  UniformCharacter& operator=(UniformCharacter const&);

 public:
  explicit UniformCharacter(size_t length);
  explicit UniformCharacter(std::string const& characters);
  UniformCharacter(size_t length, std::string const& characters);

 public:
  std::string random() const;
  std::string random(size_t length) const;
  char randomChar() const;

 private:
  size_t const _length;
  std::string const _characters;
};
}  // namespace arangodb

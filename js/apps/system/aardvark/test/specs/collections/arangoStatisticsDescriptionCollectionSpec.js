/*jslint indent: 2, nomen: true, maxlen: 100, vars: true, white: true, plusplus: true */
/*global require, exports, Backbone, window, $, arangoLog */
(function () {

    "use strict";

    describe("StatisticsDescriptionCollection", function() {

        var col;

        beforeEach(function() {
            col = new window.StatisticsDescriptionCollection();
        });

        it("StatisticsDescriptionCollection", function() {
            expect(col.model).toEqual(window.StatisticsDescription);
            expect(col.url).toEqual('/_admin/statistics-description');
            expect(col.parse("ee")).toEqual("ee");
        });
    })
}());

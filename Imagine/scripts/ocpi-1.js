var checkRoi = function () {
    //var hstart = this.hstart;
    //var hend = this.hend;
    //var vstart = this.vstart;
    //var vend = this.vend;

    //return hstart < hend && vstart < vend &&
    //    (hstart - 1) % 160 == 0 && (2560 - hend) % 160 == 0 &&
    //    vstart - 1 == 2160 - vend;
    return true;
};

var onShutterOpen = function () {
    var result = setDio(4, true);

    return result;
};


var onShutterClose = function () {
    var result = setDio(4, false);

    return result;
};


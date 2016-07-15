var checkRoi = function () {
    //var hstart = this.hstart;
    //var hend = this.hend;
    //var vstart = this.vstart;
    //var vend = this.vend;

    //return hstart < hend && vstart < vend &&
    //    (hstart - 1) % 20 == 0 && (2040 - hend) % 20 == 0 &&
    //    vstart - 1 == 2048 - vend;
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


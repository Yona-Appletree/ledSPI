var pinmap = {
	 0: {"block": "P9", "pin": 22, gpio: 2},
	 1: {"block": "P9", "pin": 21, gpio: 3},
	// 
	// -1: {"block": null, "pin": 0, "gpio": 7}, 
	// -1: {"block": null, "pin": 0, "gpio": 12},
	// -1: {"block": null, "pin": 0, "gpio": 13},
	// -1: {"block": null, "pin": 0, "gpio": 14},
	// -1: {"block": null, "pin": 0, "gpio": 15},

	 9: {"block": "P8", "pin": 24, "gpio": 33},
	10: {"block": "P8", "pin": 19, "gpio": 22},
	11: {"block": "P8", "pin": 13, "gpio": 23},
	12: {"block": "P8", "pin": 14, "gpio": 26},
	13: {"block": "P8", "pin": 17, "gpio": 27},
	14: {"block": "P9", "pin": 11, "gpio": 30},
	15: {"block": "P9", "pin": 13, "gpio": 31},
	16: {"block": "P8", "pin": 12, "gpio": 44},
	17: {"block": "P8", "pin": 11, "gpio": 45},
	18: {"block": "P8", "pin": 16, "gpio": 46},
	19: {"block": "P8", "pin": 15, "gpio": 47},
	20: {"block": "P9", "pin": 15, "gpio": 48},
	21: {"block": "P9", "pin": 23, "gpio": 49},
	22: {"block": "P9", "pin": 14, "gpio": 40},
	23: {"block": "P9", "pin": 16, "gpio": 51},
	24: {"block": "P9", "pin": 12, "gpio": 60},
	25: {"block": "P8", "pin": 18, "gpio": 65},
	26: {"block": "P8", "pin": 7,  "gpio": 66},
	27: {"block": "P8", "pin": 8,  "gpio": 67},
	28: {"block": "P8", "pin": 10, "gpio": 68},
	29: {"block": "P8", "pin": 9,  "gpio": 69},
	30: {"block": "P9", "pin": 30, "gpio": 122},
	31: {"block": "P9", "pin": 27, "gpio": 125}
};

function findPin(block, pin) {
    for (var i in pinmap) {
        var m = pinmap[i];
        if (m.block == block && m.pin == pin)
            return i;
    }
    
    return null;
}

function cell(s) {
    s = (s || "") + "";
    while (s.length < 2) s = " " + s;
    return s;
}

console.info("      [DC]     [ETHER]    [USB] ");
console.info("        P9                   P8 ");

for (var i=1; i<=46; i+=2) {
    console.info([
    	(i+1)/2,
    	"",
        i,
        i+1,
        findPin("P9", i),
        findPin("P9", i+1),
        "",
        findPin("P8", i),
        findPin("P8", i+1),
        i,
        i+1,
        "",
        (i+1)/2
    ].map(cell).join(" "));
}
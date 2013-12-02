
//     P9        P8 
//01  n  n       n  n
//02  n  n       n  n
//03  n  n       n  n
//04  n  n       y  y
//05  n  n       y  y
//06  y  n       y  y
//07  n  y       y  y
//08  y  y       y  y
//09  n  n       y  y
//10  n  n       y  n
//11  y  y       n  n
//12  y  n       n  n
//13  n  y       n  n
//14  n  n       y  n
//15  y  y       y  y
//16  y  n       y  y
//17  n  n       y  y
//18  n  n       y  y
//19  n  n       y  y
//20  n  n       y  y
//21  y  y       y  y
//22  n  n       y  y
//23  n  n       y  y
//    7  6      17 15


      

var pinData = [
		{ header: 8, headerPin:  1,   gpioNum: 0   },    
		{ header: 8, headerPin:  2, gpioNum: 0   },    
		{ header: 8, headerPin:  3, gpioNum: 38  },    
		{ header: 8, headerPin:  4, gpioNum: 39  },    
		{ header: 8, headerPin:  5, gpioNum: 34  },    
		{ header: 8, headerPin:  6, gpioNum: 35  },    
		{ header: 8, headerPin:  7, gpioNum: 66  },    
		{ header: 8, headerPin:  8, gpioNum: 67  },    
		{ header: 8, headerPin:  9, gpioNum: 69  },    
		{ header: 8, headerPin: 10, gpioNum: 68  },    
		{ header: 8, headerPin: 11, gpioNum: 45  },    
		{ header: 8, headerPin: 12, gpioNum: 44  },    
		{ header: 8, headerPin: 13, gpioNum: 23  },    
		{ header: 8, headerPin: 14, gpioNum: 26  },    
		{ header: 8, headerPin: 15, gpioNum: 47  },    
		{ header: 8, headerPin: 16, gpioNum: 46  },    
		{ header: 8, headerPin: 17, gpioNum: 27  },    
		{ header: 8, headerPin: 18, gpioNum: 65  },    
		{ header: 8, headerPin: 19, gpioNum: 22  },    
		{ header: 8, headerPin: 20, gpioNum: 63  },    
		{ header: 8, headerPin: 21, gpioNum: 62  },    
		{ header: 8, headerPin: 22, gpioNum: 37  },    
		{ header: 8, headerPin: 23, gpioNum: 36  },    
		{ header: 8, headerPin: 24, gpioNum: 33  },    
		{ header: 8, headerPin: 25, gpioNum: 1   },    
		{ header: 8, headerPin: 26, gpioNum: 61  },    
		{ header: 8, headerPin: 27, gpioNum: 86  },    
		{ header: 8, headerPin: 28, gpioNum: 88  },    
		{ header: 8, headerPin: 29, gpioNum: 87  },    
		{ header: 8, headerPin: 30, gpioNum: 89  },    
		{ header: 8, headerPin: 31, gpioNum: 10  },    
		{ header: 8, headerPin: 32, gpioNum: 11  },    
		{ header: 8, headerPin: 33, gpioNum: 9   },    
		{ header: 8, headerPin: 34, gpioNum: 81  },    
		{ header: 8, headerPin: 35, gpioNum: 8   },    
		{ header: 8, headerPin: 36, gpioNum: 80  },    
		{ header: 8, headerPin: 37, gpioNum: 78  },    
		{ header: 8, headerPin: 38, gpioNum: 79  },    
		{ header: 8, headerPin: 39, gpioNum: 76  },    
		{ header: 8, headerPin: 40, gpioNum: 77  },    
		{ header: 8, headerPin: 41, gpioNum: 74  },    
		{ header: 8, headerPin: 42, gpioNum: 75  },    
		{ header: 8, headerPin: 43, gpioNum: 72  },    
		{ header: 8, headerPin: 44, gpioNum: 73  },    
		{ header: 8, headerPin: 45, gpioNum: 70  },    
		{ header: 8, headerPin: 46, gpioNum: 71  },    
		{ header: 9, headerPin:  1, gpioNum: 0   },    
		{ header: 9, headerPin:  2, gpioNum: 0   },    
		{ header: 9, headerPin:  3, gpioNum: 0   },    
		{ header: 9, headerPin:  4, gpioNum: 0   },    
		{ header: 9, headerPin:  5, gpioNum: 0   },    
		{ header: 9, headerPin:  6, gpioNum: 0   },    
		{ header: 9, headerPin:  7, gpioNum: 0   },    
		{ header: 9, headerPin:  8, gpioNum: 0   },    
		{ header: 9, headerPin:  9, gpioNum: 0   },    
		{ header: 9, headerPin: 10, gpioNum: 0   },    
		{ header: 9, headerPin: 11, gpioNum: 30  },    
		{ header: 9, headerPin: 12, gpioNum: 60  },    
		{ header: 9, headerPin: 13, gpioNum: 31  },    
		{ header: 9, headerPin: 14, gpioNum: 50  },    
		{ header: 9, headerPin: 15, gpioNum: 48  },    
		{ header: 9, headerPin: 16, gpioNum: 51  },    
		{ header: 9, headerPin: 17, gpioNum: 5   },    
		{ header: 9, headerPin: 18, gpioNum: 4   },    
		{ header: 9, headerPin: 19, gpioNum: 13  },    
		{ header: 9, headerPin: 20, gpioNum: 12  },    
		{ header: 9, headerPin: 21, gpioNum: 3   },    
		{ header: 9, headerPin: 22, gpioNum: 2   },    
		{ header: 9, headerPin: 23, gpioNum: 49  },    
		{ header: 9, headerPin: 24, gpioNum: 15  },    
		{ header: 9, headerPin: 25, gpioNum: 117 },    
		{ header: 9, headerPin: 26, gpioNum: 14  },    
		{ header: 9, headerPin: 27, gpioNum: 115 },    
		{ header: 9, headerPin: 28, gpioNum: 113 },    
		{ header: 9, headerPin: 29, gpioNum: 111 },    
		{ header: 9, headerPin: 30, gpioNum: 112 },    
		{ header: 9, headerPin: 31, gpioNum: 110 },    
		{ header: 9, headerPin: 32, gpioNum: 0   },    
		{ header: 9, headerPin: 33, gpioNum: 0   },    
		{ header: 9, headerPin: 34, gpioNum: 0   },    
		{ header: 9, headerPin: 35, gpioNum: 0   },    
		{ header: 9, headerPin: 36, gpioNum: 0   },    
		{ header: 9, headerPin: 37, gpioNum: 0   },    
		{ header: 9, headerPin: 38, gpioNum: 0   },    
		{ header: 9, headerPin: 39, gpioNum: 0   },    
		{ header: 9, headerPin: 40, gpioNum: 0   },    
		{ header: 9, headerPin: 41, gpioNum: 20  },    
		{ header: 9, headerPin: 42, gpioNum: 7   },    
		{ header: 9, headerPin: 43, gpioNum: 0   },    
		{ header: 9, headerPin: 44, gpioNum: 0   },    
		{ header: 9, headerPin: 45, gpioNum: 0   },    
		{ header: 9, headerPin: 46, gpioNum: 0   }
];

var pinsByHeaderAndPin = {9:[], 8: []};
var pinsByGpioNum = {9:[], 8: []};
var pinsByGpioBankAndBit = {};

pinData.forEach(function(d){
	d.gpioBank = parseInt(d.gpioNum / 32);
	d.gpioBit = d.gpioNum % 32;
	d.gpioName = d.gpioBank + "_" + d.gpioBit;

	pinsByHeaderAndPin[d.header][d.headerPin] = d;
	pinsByGpioNum[d.gpioNum] = d;
	pinsByGpioBankAndBit[d.gpioBank] = pinsByGpioBankAndBit[d.gpioBank] || [];
	pinsByGpioBankAndBit[d.gpioBank][d.gpioBit] = d; 
});

var bitsUsedByBank = [
	[2, 3, /*4, 5,*/ 7, 8, 9, 10, 11, 14, 20, 22, 23, 26, 27, 30, 31],
	[12, 13, 14, 15, 16, 17, 18, 19, 28],

	[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 22, 23, /*24,*/ 25],
	[14, 15, 16, 17]
];

var channelIndex = 0;
var totalUsedPinCount = 0;
bitsUsedByBank.forEach(function(usedInBank, bankNum){
	usedInBank.forEach(function(bitNum){
		totalUsedPinCount ++;
		pinsByGpioBankAndBit[bankNum][bitNum].used = true;
		pinsByGpioBankAndBit[bankNum][bitNum].channelIndex = channelIndex++;
	});
})

var p8verified = [ // p8
	0,  0, // 1
	0,  0, // 2
	0,  0, // 3
	1,  1, // 4
	1,  1, // 5
	1,  1, // 6
	1,  1, // 7
	1,  1, // 8
	1,  1, // 9
	1,  0, // 10
	0,  0, // 11
	0,  0, // 12
	0,  0, // 13
	1,  0, // 14
	1,  1, // 15
	1,  1, // 16
	1,  1, // 17
	1,  1, // 18
	1,  1, // 19
	1,  1, // 20
	1,  1, // 21
	1,  1, // 22
	1,  1, // 23
];

var p9verified = [ // P9
	0,  0, // 1
	0,  0, // 2
	0,  0, // 3
	0,  0, // 4
	0,  0, // 5
	1,  1, // 6
	1,  1, // 7
	1,  1, // 8
	0,  0, // 9
	0,  0, // 10
	1,  1, // 11
	1,  0, // 12
	0,  1, // 13
	0,  1, // 14
	1,  1, // 15
	1,  0, // 16
	0,  0, // 17
	0,  0, // 18
	0,  0, // 19
	0,  0, // 20
	1,  1, // 21
	0,  0, // 22
	0,  0, // 23
];

var totalVerifiedCount = 0;
p8verified.forEach(function(verified, i){pinsByHeaderAndPin[8][i+1].verified = !!verified; totalVerifiedCount += verified?1:0; })
p9verified.forEach(function(verified, i){pinsByHeaderAndPin[9][i+1].verified = !!verified; totalVerifiedCount += verified?1:0; })


var Color = {
	black: 30
  , blue: 34
  , cyan: 36
  , green: 32
  , magenta: 35
  , red: 31
  , yellow: 33
  , grey: 90
  , brightBlack: 90
  , brightRed: 91
  , brightGreen: 92
  , brightYellow: 93
  , brightBlue: 94
  , brightMagenta: 95
  , brightCyan: 96
  , brightWhite: 97
}

var defaultCellSize = 5;
var rowStr = "";
function cell(str, color, cellSize) {
	color = color || null;
	cellSize = cellSize || defaultCellSize;

	str = "" + str;
	var paddingHalf = (cellSize - str.length) / 2;
	for (var i=0; i<Math.ceil(paddingHalf); i++) { str = " " + str };
	for (var i=0; i<Math.floor(paddingHalf); i++) { str = str + " " };

	if (color) { rowStr += "\x1b[01;" + color + "m"; }
	rowStr += str;
	if (color) { rowStr += "\x1b[0m"; }
}

function endRow() {
	console.info(rowStr);
	rowStr = "";
}

function printTable(title, f) {
	var headerColumnsWidth = defaultCellSize*3 + 8*2;
	cell(title, Color.brightBlue, headerColumnsWidth*2 + defaultCellSize); 
	endRow();

	cell("Row", Color.brightMagenta);
	cell("Pin#", Color.brightMagenta);
	cell("P9", Color.blue, 16);
	cell("Pin#", Color.brightMagenta);
	cell("|", null, 4);
	cell("Pin#", Color.brightMagenta);
	cell("P8", Color.blue, 16);
	cell("Pin#", Color.brightMagenta);
	cell("Row", Color.brightMagenta);
	endRow();

	for (var row=0; row<23; row++) {
		cell(row+1, Color.cyan);
		cell(row*2+1, Color.yellow);
		cell(f(pinsByHeaderAndPin[9][row*2+1]), Color.brightGreen, 8);
		cell(f(pinsByHeaderAndPin[9][row*2+2]), Color.brightGreen, 8);
		cell(row*2+2, Color.yellow);
		cell("|", null, 4);
		cell(row*2+1, Color.yellow);
		cell(f(pinsByHeaderAndPin[8][row*2+1]), Color.brightGreen, 8);
		cell(f(pinsByHeaderAndPin[8][row*2+2]), Color.brightGreen, 8);
		cell(row*2+2, Color.yellow);
		cell(row+1, Color.cyan);
		endRow();
	}
	endRow();
}

printTable("GPIO: BANK_BIT", function(p){ return p.gpioNum ? p.gpioName : "" });
printTable("GPIO: Global Number", function(p){ return p.gpioNum || "" });
printTable("LEDscape Channel Index", function(p){ return p.channelIndex!=undefined ? p.channelIndex : "" });

printTable("Non-verified used pins", function(p){ return (!p.verified && p.used) ? p.gpioName : "" });
console.info("Total Used Pins: " + totalVerifiedCount);
console.info("Total Verified Pins: " + totalUsedPinCount);

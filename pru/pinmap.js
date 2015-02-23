var pinData = [
	{ header: 8, headerPin:  1, gpioNum: 0  , name: "GND"        },
	{ header: 8, headerPin:  2, gpioNum: 0  , name: "GND"        },
	{ header: 8, headerPin:  3, gpioNum: 38 , name: "GPIO1_6"    },
	{ header: 8, headerPin:  4, gpioNum: 39 , name: "GPIO1_7"    },
	{ header: 8, headerPin:  5, gpioNum: 34 , name: "GPIO1_2"    },
	{ header: 8, headerPin:  6, gpioNum: 35 , name: "GPIO1_3"    },
	{ header: 8, headerPin:  7, gpioNum: 66 , name: "TIMER4"     },
	{ header: 8, headerPin:  8, gpioNum: 67 , name: "TIMER7"     },
	{ header: 8, headerPin:  9, gpioNum: 69 , name: "TIMER5"     },
	{ header: 8, headerPin: 10, gpioNum: 68 , name: "TIMER6"     },
	{ header: 8, headerPin: 11, gpioNum: 45 , name: "GPIO1_13"   },
	{ header: 8, headerPin: 12, gpioNum: 44 , name: "GPIO1_12"   },
	{ header: 8, headerPin: 13, gpioNum: 23 , name: "EHRPWM2B"   },
	{ header: 8, headerPin: 14, gpioNum: 26 , name: "GPIO0_26"   },
	{ header: 8, headerPin: 15, gpioNum: 47 , name: "GPIO1_15"   },
	{ header: 8, headerPin: 16, gpioNum: 46 , name: "GPIO1_14"   },
	{ header: 8, headerPin: 17, gpioNum: 27 , name: "GPIO0_27"   },
	{ header: 8, headerPin: 18, gpioNum: 65 , name: "GPIO2_1"    },
	{ header: 8, headerPin: 19, gpioNum: 22 , name: "EHRPWM2A"   },
	{ header: 8, headerPin: 20, gpioNum: 63 , name: "GPIO1_31"   },
	{ header: 8, headerPin: 21, gpioNum: 62 , name: "GPIO1_30"   },
	{ header: 8, headerPin: 22, gpioNum: 37 , name: "GPIO1_5"    },
	{ header: 8, headerPin: 23, gpioNum: 36 , name: "GPIO1_4"    },
	{ header: 8, headerPin: 24, gpioNum: 33 , name: "GPIO1_1"    },
	{ header: 8, headerPin: 25, gpioNum: 1  , name: "GPIO1_0"    },
	{ header: 8, headerPin: 26, gpioNum: 61 , name: "GPIO1_29"   },
	{ header: 8, headerPin: 27, gpioNum: 86 , name: "GPIO2_22"   },
	{ header: 8, headerPin: 28, gpioNum: 88 , name: "GPIO2_24"   },
	{ header: 8, headerPin: 29, gpioNum: 87 , name: "GPIO2_23"   },
	{ header: 8, headerPin: 30, gpioNum: 89 , name: "GPIO2_25"   },
	{ header: 8, headerPin: 31, gpioNum: 10 , name: "UART5_CTSN" },
	{ header: 8, headerPin: 32, gpioNum: 11 , name: "UART5_RTSN" },
	{ header: 8, headerPin: 33, gpioNum: 9  , name: "UART4_RTSN" },
	{ header: 8, headerPin: 34, gpioNum: 81 , name: "UART3_RTSN" },
	{ header: 8, headerPin: 35, gpioNum: 8  , name: "UART4_CTSN" },
	{ header: 8, headerPin: 36, gpioNum: 80 , name: "UART3_CTSN" },
	{ header: 8, headerPin: 37, gpioNum: 78 , name: "UART5_TXD"  },
	{ header: 8, headerPin: 38, gpioNum: 79 , name: "UART5_RXD"  },
	{ header: 8, headerPin: 39, gpioNum: 76 , name: "GPIO2_12"   },
	{ header: 8, headerPin: 40, gpioNum: 77 , name: "GPIO2_13"   },
	{ header: 8, headerPin: 41, gpioNum: 74 , name: "GPIO2_10"   },
	{ header: 8, headerPin: 42, gpioNum: 75 , name: "GPIO2_11"   },
	{ header: 8, headerPin: 43, gpioNum: 72 , name: "GPIO2_8"    },
	{ header: 8, headerPin: 44, gpioNum: 73 , name: "GPIO2_9"    },
	{ header: 8, headerPin: 45, gpioNum: 70 , name: "GPIO2_6"    },
	{ header: 8, headerPin: 46, gpioNum: 71 , name: "GPIO2_7"    },
	{ header: 9, headerPin:  1, gpioNum: 0  , name: "GND"        },
	{ header: 9, headerPin:  2, gpioNum: 0  , name: "GND"        },
	{ header: 9, headerPin:  3, gpioNum: 0  , name: "VDD_3V3EXP" },
	{ header: 9, headerPin:  4, gpioNum: 0  , name: "VDD_3V3EXP" },
	{ header: 9, headerPin:  5, gpioNum: 0  , name: "VDD_5V"     },
	{ header: 9, headerPin:  6, gpioNum: 0  , name: "VDD_5V"     },
	{ header: 9, headerPin:  7, gpioNum: 0  , name: "SYS_5V"     },
	{ header: 9, headerPin:  8, gpioNum: 0  , name: "SYS_5V"     },
	{ header: 9, headerPin:  9, gpioNum: 0  , name: "PWR_BUT"    },
	{ header: 9, headerPin: 10, gpioNum: 0  , name: "SYS_RESETn" },
	{ header: 9, headerPin: 11, gpioNum: 30 , name: "UART4_RXD"  },
	{ header: 9, headerPin: 12, gpioNum: 60 , name: "GPIO1_28"   },
	{ header: 9, headerPin: 13, gpioNum: 31 , name: "UART4_TXD"  },
	{ header: 9, headerPin: 14, gpioNum: 50 , name: "EHRPWM1A"   },
	{ header: 9, headerPin: 15, gpioNum: 48 , name: "GPIO1_16"   },
	{ header: 9, headerPin: 16, gpioNum: 51 , name: "EHRPWM1B"   },
	{ header: 9, headerPin: 17, gpioNum: 5  , name: "I2C1_SCL"   },
	{ header: 9, headerPin: 18, gpioNum: 4  , name: "I2C1_SDA"   },
	{ header: 9, headerPin: 19, gpioNum: 13 , name: "I2C2_SCL"   },
	{ header: 9, headerPin: 20, gpioNum: 12 , name: "I2C2_SDA"   },
	{ header: 9, headerPin: 21, gpioNum: 3  , name: "UART2_TXD"  },
	{ header: 9, headerPin: 22, gpioNum: 2  , name: "UART2_RXD"  },
	{ header: 9, headerPin: 23, gpioNum: 49 , name: "GPIO1_17"   },
	{ header: 9, headerPin: 24, gpioNum: 15 , name: "UART1_TXD"  },
	{ header: 9, headerPin: 25, gpioNum: 117, name: "GPIO3_21"   },
	{ header: 9, headerPin: 26, gpioNum: 14 , name: "UART1_RXD"  },
	{ header: 9, headerPin: 27, gpioNum: 115, name: "GPIO3_19"   },
	{ header: 9, headerPin: 28, gpioNum: 113, name: "SPI1_CS0"   },
	{ header: 9, headerPin: 29, gpioNum: 111, name: "SPI1_MISO"  }, // Was SPI1_D0 (Input)
	{ header: 9, headerPin: 30, gpioNum: 112, name: "SPI1_MOSI"  }, // Was SPI1_D1 (Output)
	{ header: 9, headerPin: 31, gpioNum: 110, name: "SPI1_CLK"   },
	{ header: 9, headerPin: 32, gpioNum: 0  , name: "VDD_ADC"    },
	{ header: 9, headerPin: 33, gpioNum: 0  , name: "AIN4"       },
	{ header: 9, headerPin: 34, gpioNum: 0  , name: "GNDA_ADC"   },
	{ header: 9, headerPin: 35, gpioNum: 0  , name: "AIN6"       },
	{ header: 9, headerPin: 36, gpioNum: 0  , name: "AIN5"       },
	{ header: 9, headerPin: 37, gpioNum: 0  , name: "AIN2"       },
	{ header: 9, headerPin: 38, gpioNum: 0  , name: "AIN3"       },
	{ header: 9, headerPin: 39, gpioNum: 0  , name: "AIN0"       },
	{ header: 9, headerPin: 40, gpioNum: 0  , name: "AIN1"       },
	{ header: 9, headerPin: 41, gpioNum: 20 , name: "CLKOUT2"    },
	{ header: 9, headerPin: 42, gpioNum: 7  , name: "GPIO1_7"    },
	{ header: 9, headerPin: 43, gpioNum: 0  , name: "GND"        },
	{ header: 9, headerPin: 44, gpioNum: 0  , name: "GND"        },
	{ header: 9, headerPin: 45, gpioNum: 0  , name: "GND"        },
	{ header: 9, headerPin: 46, gpioNum: 0  , name: "GND"        }
];


var pinsByHeaderAndPin = {9:[], 8: []};
var pinsByGpioNum = {9:[], 8: []};
var pinsByGpioBankAndBit = {};
var pinsByPruChannel = {};

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

		pinsByPruChannel[channelIndex] = pinsByGpioBankAndBit[bankNum][bitNum];

		pinsByGpioBankAndBit[bankNum][bitNum].used = true;
		pinsByGpioBankAndBit[bankNum][bitNum].channelIndex = channelIndex++;
	});
});

var pinsByMappedChannelIndex = [];

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

function printPinTable(title, f) {
	var headerColumnsWidth = defaultCellSize*3 + 8*2;
	cell(title, Color.brightBlue, headerColumnsWidth*2 + defaultCellSize);
	endRow();

	cell("Row", Color.brightMagenta);
	cell("Pin#", Color.brightMagenta);
	cell("P9", Color.blue, 24);
	cell("Pin#", Color.brightMagenta);
	cell("|", null, 4);
	cell("Pin#", Color.brightMagenta);
	cell("P8", Color.blue, 24);
	cell("Pin#", Color.brightMagenta);
	cell("Row", Color.brightMagenta);
	endRow();

	for (var row=0; row<23; row++) {
		cell(row+1, Color.cyan);
		cell(row*2+1, Color.yellow);
		cell(f(pinsByHeaderAndPin[9][row*2+1]), Color.brightGreen, 12);
		cell(f(pinsByHeaderAndPin[9][row*2+2]), Color.brightGreen, 12);
		cell(row*2+2, Color.yellow);
		cell("|", null, 4);
		cell(row*2+1, Color.yellow);
		cell(f(pinsByHeaderAndPin[8][row*2+1]), Color.brightGreen, 12);
		cell(f(pinsByHeaderAndPin[8][row*2+2]), Color.brightGreen, 12);
		cell(row*2+2, Color.yellow);
		cell(row+1, Color.cyan);
		endRow();
	}
	endRow();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Program Commands
var Commands = {
	"tables": function(){
		printPinTable("GPIO: BANK_BIT", function(p){ return p.gpioNum ? p.gpioName : "" });
		printPinTable("GPIO: Global Number", function(p){ return p.gpioNum || "" });
		printPinTable("Original Channel Index", function(p){ return p.channelIndex!=undefined ? p.channelIndex : "" });
		printPinTable("Mapped Channel Index", function(p){ return p.mappedChannelIndex!=undefined ? p.mappedChannelIndex : "" });

		printPinTable("Unused Channels", function(p){ return (p.channelIndex==undefined & p.gpioNum) ? p.gpioName : "" });
		printPinTable("NAME", function(p){ return p.name });

//printPinTable("Non-verified used pins", function(p){ return (!p.verified && p.used) ? p.gpioName : "" });
		console.info("Total Used Pins: " + totalVerifiedCount);
		console.info("Total Verified Pins: " + totalUsedPinCount);
	},

	"pinout": function(){
		printPinTable("Internal Channel Index", function(p){ return p.mappedChannelIndex!=undefined ? p.mappedChannelIndex : "" });
	},

	"pru-headers": function() {
		function buildRangeHeader(prefix, startIndexInclusive, endIndexExclusive) {
			var output = "";

			var bankData = [];
			for (var i=0; i<4; i++) {
				bankData[i] = {
					usedPinBitsAndIndexes: [],
					usePin: function(channelIndex, gpioBit) {
						this.usedPinBitsAndIndexes.push({ channelIndex: channelIndex, gpioBit: gpioBit });
						return this.usedPinBitsAndIndexes.length - 1;
					},
					pinMaskFor: function(divisor, inverse) {
						inverse = !! inverse;

						return this.usedPinBitsAndIndexes
							.filter(function(used){ return (used.channelIndex%divisor==0) != inverse; })
							.reduce(function(mask, used){ return mask | (1<<used.gpioBit); }, 0)
					}
				};
			}

			for (var channelIndex=startIndexInclusive, relativeIndex=0; channelIndex<endIndexExclusive; channelIndex++, relativeIndex++) {
				var pin = pinsByMappedChannelIndex[channelIndex];
				if (pin) {
					var usedBitIndexInBank = bankData[pin.gpioBank].usePin(channelIndex, pin.gpioBit);
					output += "// --- Channel " + channelIndex + " ---\n";
					output += "#define " + prefix + "gpio" + pin.gpioBank + "_bit" + usedBitIndexInBank + " " + pin.gpioBit + "\n";
					output += "#define " + prefix + "channel" + relativeIndex + "_bank " + pin.gpioBank + "\n";
					output += "#define " + prefix + "channel" + relativeIndex + "_bit " + pin.gpioBit + "\n";
					output += "#define " + prefix + "channel" + relativeIndex + "_usedBit " + usedBitIndexInBank + "\n";
					output += "\n";
				}
			}

			function toHexLiteral(n) {
				if (n < 0) {
					n = 0xFFFFFFFF + n + 1;
				}
				var s =  n.toString(16).toUpperCase();
				while (s.length % 8 != 0) {
					s = "0" + s;
				}
				return "0x" + s;
			}

			output += "\n";
			bankData.forEach(function(bank, bankIndex){
				output += "#define " + prefix + "gpio" + bankIndex + "_all_mask " + toHexLiteral(bank.pinMaskFor(1)) + "\n";
				output += "#define " + prefix + "gpio" + bankIndex + "_even_mask " + toHexLiteral(bank.pinMaskFor(2)) + "\n";
				output += "#define " + prefix + "gpio" + bankIndex + "_odd_mask " + toHexLiteral(bank.pinMaskFor(2, true)) + "\n";
			});

			return output;
		}

		var slashLine = (function(){
			var s = "";
			while (s.length < 120) s += "/";
			return s;
		})();

		console.info(slashLine);
		console.info("// Pin Mapping: " + pinMapping.name);
		console.info(slashLine);

		console.info(slashLine);
		console.info("// PRU0 Mappings");
		console.info(buildRangeHeader("pru0_", 0, 24));
		console.info(slashLine);
		console.info("\n\n");
		console.info(slashLine);
		console.info("// PRU1 Mappings");
		console.info(buildRangeHeader("pru1_", 24, 48));
		console.info(slashLine);
	}
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Bootstrap
var mappingFilename = "original-ledscape";

function usage(error) {
	if (error) {
		console.error(error);
		console.info();
	}
	console.info("Usage: " + process.execPath + " [--mapping mappingFile] [tables | pru-headers]");
	process.exit(error ? -1 : 0);
}

var commandFunc = Commands.pinout;
process.argv.forEach(function(arg, i) {
	if (arg === "-h" || arg === "-?") {
		usage();
	}

	if (arg === "--mapping") {
		if (process.argv.length > i+1) {
			mappingFilename = process.argv[i + 1];
		} else {
			usage(arg + " requires an argument");
		}
	}

	if (arg in Commands) {
		commandFunc = Commands[arg];
	}
});

var fs = require('fs');
var path = require('path');

// Look for the mapping file in various places... allow the name of the mapping with or without an extension and
// allow references to the mappings in the relative directory mappings/
var validPaths = [
	mappingFilename,
	mappingFilename + ".json",
	path.dirname(require.main.filename) + "/mappings/" + mappingFilename,
	path.dirname(require.main.filename) + "/mappings/" + mappingFilename + ".json"
].filter(fs.existsSync);

if (validPaths.length == 0) {
	usage("Could not find mapping: " + mappingFilename);
}

var pinMapping = JSON.parse(fs.readFileSync(validPaths[0], "utf8"));

process.stderr.write("Using mapping: " + pinMapping.name + " from " + mappingFilename + "\n");
if (pinMapping.mappedPinNumberToOriginalPinNumberMap) {
	for (var i = 0; i<totalUsedPinCount; i++) {
		var pin = pinsByPruChannel[pinMapping.mappedPinNumberToOriginalPinNumberMap[i]];
		pinsByMappedChannelIndex[i] = pin;
		pin.mappedChannelIndex = i;
	}

	commandFunc();
} else {
	usage("Invalid mapping file format. No mappedPinNumberToOriginalPinNumberMap field found.");
}

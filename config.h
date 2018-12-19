/* Vega Bar config file
 * -> contains variable global constants
 * -> like fonts and color schemes
 *
 *
 * */

#define fontsperset 2


static const char* vegabarcolor[SchemeLast][2] = {	[SchemeNorm] = 	{"#dddddd", "#333333"},
													[SchemeSel] = 	{"#ff0000", "#0000ff"},
};

static const char* vegabarfonts[FontsLast][fontsperset]	 = {	[FontsNorm] = { "SFNS Display:size=24",	"Font Awesome Free Regular:size=24" },
																[FontsBig]	= { "SNFS Display:size=52", "Font Awesome Free Regular:size=52" },
};


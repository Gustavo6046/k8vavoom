//**************************************************************************
//**
//**	##   ##    ##    ##   ##   ####     ####   ###     ###
//**	##   ##  ##  ##  ##   ##  ##  ##   ##  ##  ####   ####
//**	 ## ##  ##    ##  ## ##  ##    ## ##    ## ## ## ## ##
//**	 ## ##  ########  ## ##  ##    ## ##    ## ##  ###  ##
//**	  ###   ##    ##   ###    ##  ##   ##  ##  ##       ##
//**	   #    ##    ##    #      ####     ####   ##       ##
//**
//**	$Id$
//**
//**	Copyright (C) 1999-2002 J�nis Legzdi��
//**
//**	This program is free software; you can redistribute it and/or
//**  modify it under the terms of the GNU General Public License
//**  as published by the Free Software Foundation; either version 2
//**  of the License, or (at your option) any later version.
//**
//**	This program is distributed in the hope that it will be useful,
//**  but WITHOUT ANY WARRANTY; without even the implied warranty of
//**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//**  GNU General Public License for more details.
//**
//**************************************************************************

char *statename[] = {
	"S_NULL",
	"S_LIGHTDONE",
	"S_WAVE1",
	"S_WAVE2",
	"S_WAVE3",
	"S_WAVE4",
	"S_6",
	"S_7",
	"S_8",
	"S_9",
	"S_TARGETER1",
	"S_TARGETER2",
	"S_TARGETER3",
	"S_PUNCH",
	"S_PUNCHDOWN",
	"S_PUNCHUP",
	"S_PUNCH1",
	"S_PUNCH2",
	"S_PUNCH3",
	"S_PUNCH4",
	"S_PUNCH5",
	"S_XBOW",
	"S_XBOWDOWN",
	"S_XBOWUP",
	"S_XBOW1",
	"S_XBOW2",
	"S_XBOW3",
	"S_XBOW4",
	"S_XBOW5",
	"S_XBOW6",
	"S_XBOW7",
	"S_XBOWFLASH1",
	"S_XBOWFLASH2",
	"S_XBOWFLASH3",
	"S_XBOWPOISON",
	"S_XBOWPOISONDOWN",
	"S_XBOWPOISONUP",
	"S_XBOWPOISON1",
	"S_XBOWPOISON2",
	"S_XBOWPOISON3",
	"S_XBOWPOISON4",
	"S_XBOWPOISON5",
	"S_XBOWPOISON6",
	"S_XBOWPOISON7",
	"S_MISSILE",
	"S_MISSILEDOWN",
	"S_MISSILEUP",
	"S_MISSILE1",
	"S_MISSILE2",
	"S_MISSILE3",
	"S_MISSILE4",
	"S_MISSILE5",
	"S_MISSILE6",
	"S_MISSILE7",
	"S_RIFFLE",
	"S_RIFFLEDOWN",
	"S_RIFFLEUP",
	"S_RIFFLE1",
	"S_RIFFLE2",
	"S_RIFFLE3",
	"S_RIFFLE4",
	"S_RIFFLE5",
	"S_FLAME",
	"S_FLAME01",
	"S_FLAMEDOWN",
	"S_FLAMEUP",
	"S_FLAME1",
	"S_FLAME2",
	"S_BLASTER",
	"S_BLASTER01",
	"S_BLASTER02",
	"S_BLASTER03",
	"S_BLASTERDOWN",
	"S_BLASTERUP",
	"S_BLASTER1",
	"S_BLASTER2",
	"S_BLASTER3",
	"S_BLASTER4",
	"S_BLASTER5",
	"S_BLASTER6",
	"S_BLASTER7",
	"S_BLASTER8",
	"S_BLASTERTORPEDO",
	"S_BLASTERTORPEDO01",
	"S_BLASTERTORPEDO02",
	"S_BLASTERTORPEDO03",
	"S_BLASTERTORPEDODOWN",
	"S_BLASTERTORPEDOUP",
	"S_BLASTERTORPEDO1",
	"S_BLASTERTORPEDO2",
	"S_BLASTERTORPEDO3",
	"S_BLASTERTORPEDO4",
	"S_BLASTERTORPEDO5",
	"S_BLASTERTORPEDO6",
	"S_BLASTERTORPEDO7",
	"S_GRENADE",
	"S_GRENADEDOWN",
	"S_GRENADEUP",
	"S_GRENADE1",
	"S_GRENADE2",
	"S_GRENADE3",
	"S_GRENADE4",
	"S_GRENADE5",
	"S_GRENADEFLASH1",
	"S_GRENADEFLASH2",
	"S_GRENADEFLASH3",
	"S_GRENADEHIGH",
	"S_GRENADEHIGHDOWN",
	"S_GRENADEHIGHUP",
	"S_GRENADEHIGH1",
	"S_GRENADEHIGH2",
	"S_GRENADEHIGH3",
	"S_GRENADEHIGH4",
	"S_GRENADEHIGH5",
	"S_GRENADEHIGHFLASH1",
	"S_GRENADEHIGHFLASH2",
	"S_GRENADEHIGHFLASH3",
	"S_SIGIL",
	"S_SIGIL01",
	"S_SIGIL02",
	"S_SIGIL03",
	"S_SIGIL04",
	"S_SIGILDOWN",
	"S_SIGILUP",
	"S_SIGIL1",
	"S_SIGIL2",
	"S_SIGIL3",
	"S_SIGIL4",
	"S_SIGILFLASH1",
	"S_SIGILFLASH2",
	"S_SIGILFLASH3",
	"S_131",
	"S_132",
	"S_133",
	"S_134",
	"S_135",
	"S_136",
	"S_137",
	"S_138",
	"S_139",
	"S_140",
	"S_141",
	"S_142",
	"S_143",
	"S_144",
	"S_145",
	"S_146",
	"S_147",
	"S_148",
	"S_149",
	"S_150",
	"S_151",
	"S_152",
	"S_153",
	"S_154",
	"S_155",
	"S_156",
	"S_157",
	"S_158",
	"S_159",
	"S_160",
	"S_161",
	"S_162",
	"S_163",
	"S_164",
	"S_165",
	"S_BLOOD1",
	"S_BLOOD2",
	"S_BLOOD3",
	"S_PUFF1",
	"S_PUFF2",
	"S_PUFF3",
	"S_PUFF4",
	"S_173",
	"S_174",
	"S_175",
	"S_176",
	"S_177",
	"S_178",
	"S_179",
	"S_180",
	"S_181",
	"S_182",
	"S_183",
	"S_184",
	"S_185",
	"S_186",
	"S_187",
	"S_188",
	"S_189",
	"S_190",
	"S_191",
	"S_192",
	"S_193",
	"S_194",
	"S_195",
	"S_196",
	"S_197",
	"S_198",
	"S_199",
	"S_200",
	"S_201",
	"S_202",
	"S_203",
	"S_204",
	"S_205",
	"S_206",
	"S_207",
	"S_208",
	"S_209",
	"S_210",
	"S_211",
	"S_212",
	"S_213",
	"S_214",
	"S_215",
	"S_216",
	"S_217",
	"S_218",
	"S_219",
	"S_220",
	"S_221",
	"S_222",
	"S_223",
	"S_224",
	"S_225",
	"S_226",
	"S_227",
	"S_228",
	"S_229",
	"S_230",
	"S_231",
	"S_232",
	"S_233",
	"S_234",
	"S_235",
	"S_236",
	"S_237",
	"S_238",
	"S_239",
	"S_240",
	"S_241",
	"S_242",
	"S_243",
	"S_244",
	"S_245",
	"S_246",
	"S_247",
	"S_248",
	"S_249",
	"S_250",
	"S_251",
	"S_252",
	"S_253",
	"S_254",
	"S_255",
	"S_256",
	"S_257",
	"S_258",
	"S_259",
	"S_260",
	"S_261",
	"S_262",
	"S_263",
	"S_264",
	"S_265",
	"S_266",
	"S_267",
	"S_268",
	"S_TFOG1",
	"S_TFOG2",
	"S_TFOG3",
	"S_TFOG4",
	"S_TFOG5",
	"S_TFOG6",
	"S_TFOG7",
	"S_TFOG8",
	"S_TFOG9",
	"S_TFOG10",
	"S_IFOG1",
	"S_IFOG2",
	"S_IFOG3",
	"S_IFOG4",
	"S_IFOG5",
	"S_IFOG6",
	"S_IFOG7",
	"S_286",
	"S_287",
	"S_288",
	"S_PLAY",
	"S_PLAY_RUN1",
	"S_PLAY_RUN2",
	"S_PLAY_RUN3",
	"S_PLAY_RUN4",
	"S_PLAY_ATK1",
	"S_PLAY_ATK2",
	"S_PLAY_PAIN",
	"S_PLAY_PAIN2",
	"S_PLAY_DIE1",
	"S_PLAY_DIE2",
	"S_PLAY_DIE3",
	"S_PLAY_DIE4",
	"S_PLAY_DIE5",
	"S_PLAY_DIE6",
	"S_PLAY_DIE7",
	"S_PLAY_DIE8",
	"S_PLAY_DIE9",
	"S_PLAY_DIE10",
	"S_PLAY_XDIE1",
	"S_PLAY_XDIE2",
	"S_PLAY_XDIE3",
	"S_PLAY_XDIE4",
	"S_PLAY_XDIE5",
	"S_PLAY_XDIE6",
	"S_PLAY_XDIE7",
	"S_PLAY_XDIE8",
	"S_MERCHANDER_YES1",
	"S_MERCHANDER_NO1",
	"S_MERCHANDER_NO2",
	"S_MERCHANDER_NO3",
	"S_MERCHANDER_NO4",
	"S_MERCHANDER_NO5",
	"S_MERCHANDER_STAND1",
	"S_MERCHANDER_LOOK1",
	"S_MERCHANDER_LOOK2",
	"S_MERCHANDER_BD1",
	"S_MERCHANDER_BD2",
	"S_MERCHANDER_BD3",
	"S_MERCHANDER_BD4",
	"S_MERCHANDER_BD5",
	"S_MERCHANDER_BD6",
	"S_MERCHANDER_BD7",
	"S_MERCHANDER_BD8",
	"S_MERCHANDER_BD9",
	"S_MERCHANDER_BD10",
	"S_MERCHANDER_PAIN1",
	"S_MERCHANDER_PAIN2",
	"S_MERCHANDER_PAIN3",
	"S_MERCHANDER_PAIN4",
	"S_MERCHANDER_PAIN5",
	"S_MERCHANDER_PAIN6",
	"S_MERCHANDER_PAIN7",
	"S_MERCHANDER_GT1",
	"S_MERCHANDER_GT2",
	"S_MERCHANDER_GT3",
	"S_MERCHANDER_GT4",
	"S_MERCHANDER_GT5",
	"S_MERCHANDER_GT6",
	"S_MERCHANDER_GT7",
	"S_MERCHANDER_GT8",
	"S_MERCHANDER_GT9",
	"S_351",
	"S_352",
	"S_353",
	"S_354",
	"S_355",
	"S_356",
	"S_357",
	"S_358",
	"S_359",
	"S_360",
	"S_361",
	"S_362",
	"S_363",
	"S_364",
	"S_365",
	"S_366",
	"S_367",
	"S_368",
	"S_369",
	"S_370",
	"S_371",
	"S_372",
	"S_373",
	"S_374",
	"S_375",
	"S_376",
	"S_377",
	"S_378",
	"S_379",
	"S_380",
	"S_381",
	"S_382",
	"S_383",
	"S_384",
	"S_385",
	"S_386",
	"S_387",
	"S_388",
	"S_389",
	"S_390",
	"S_391",
	"S_392",
	"S_393",
	"S_394",
	"S_395",
	"S_396",
	"S_397",
	"S_398",
	"S_399",
	"S_400",
	"S_401",
	"S_402",
	"S_403",
	"S_404",
	"S_405",
	"S_406",
	"S_407",
	"S_408",
	"S_409",
	"S_410",
	"S_411",
	"S_412",
	"S_413",
	"S_414",
	"S_415",
	"S_416",
	"S_417",
	"S_418",
	"S_419",
	"S_420",
	"S_421",
	"S_422",
	"S_423",
	"S_424",
	"S_425",
	"S_426",
	"S_427",
	"S_428",
	"S_429",
	"S_430",
	"S_431",
	"S_432",
	"S_433",
	"S_434",
	"S_435",
	"S_436",
	"S_437",
	"S_438",
	"S_439",
	"S_440",
	"S_441",
	"S_442",
	"S_443",
	"S_444",
	"S_445",
	"S_446",
	"S_447",
	"S_448",
	"S_449",
	"S_450",
	"S_451",
	"S_452",
	"S_453",
	"S_454",
	"S_455",
	"S_456",
	"S_457",
	"S_458",
	"S_459",
	"S_460",
	"S_461",
	"S_462",
	"S_463",
	"S_464",
	"S_465",
	"S_466",
	"S_467",
	"S_468",
	"S_469",
	"S_470",
	"S_471",
	"S_472",
	"S_473",
	"S_474",
	"S_475",
	"S_476",
	"S_477",
	"S_478",
	"S_479",
	"S_480",
	"S_481",
	"S_482",
	"S_483",
	"S_484",
	"S_485",
	"S_486",
	"S_487",
	"S_488",
	"S_489",
	"S_490",
	"S_491",
	"S_492",
	"S_493",
	"S_494",
	"S_495",
	"S_496",
	"S_497",
	"S_498",
	"S_499",
	"S_500",
	"S_501",
	"S_502",
	"S_503",
	"S_504",
	"S_505",
	"S_506",
	"S_507",
	"S_508",
	"S_509",
	"S_510",
	"S_511",
	"S_512",
	"S_513",
	"S_514",
	"S_515",
	"S_516",
	"S_517",
	"S_518",
	"S_519",
	"S_520",
	"S_521",
	"S_522",
	"S_523",
	"S_524",
	"S_525",
	"S_526",
	"S_527",
	"S_528",
	"S_529",
	"S_530",
	"S_531",
	"S_532",
	"S_533",
	"S_534",
	"S_535",
	"S_536",
	"S_537",
	"S_538",
	"S_539",
	"S_540",
	"S_541",
	"S_542",
	"S_543",
	"S_544",
	"S_545",
	"S_546",
	"S_547",
	"S_548",
	"S_549",
	"S_550",
	"S_551",
	"S_552",
	"S_553",
	"S_554",
	"S_555",
	"S_556",
	"S_557",
	"S_558",
	"S_559",
	"S_560",
	"S_561",
	"S_562",
	"S_563",
	"S_564",
	"S_565",
	"S_566",
	"S_567",
	"S_568",
	"S_569",
	"S_570",
	"S_571",
	"S_572",
	"S_573",
	"S_574",
	"S_575",
	"S_576",
	"S_577",
	"S_578",
	"S_579",
	"S_580",
	"S_581",
	"S_582",
	"S_583",
	"S_584",
	"S_585",
	"S_586",
	"S_587",
	"S_588",
	"S_589",
	"S_590",
	"S_591",
	"S_592",
	"S_593",
	"S_594",
	"S_595",
	"S_596",
	"S_597",
	"S_598",
	"S_599",
	"S_600",
	"S_601",
	"S_602",
	"S_603",
	"S_604",
	"S_605",
	"S_606",
	"S_607",
	"S_608",
	"S_609",
	"S_610",
	"S_611",
	"S_612",
	"S_613",
	"S_614",
	"S_615",
	"S_616",
	"S_617",
	"S_618",
	"S_619",
	"S_620",
	"S_621",
	"S_622",
	"S_623",
	"S_624",
	"S_625",
	"S_626",
	"S_627",
	"S_628",
	"S_629",
	"S_630",
	"S_631",
	"S_632",
	"S_633",
	"S_634",
	"S_635",
	"S_636",
	"S_637",
	"S_638",
	"S_639",
	"S_640",
	"S_641",
	"S_642",
	"S_643",
	"S_644",
	"S_645",
	"S_646",
	"S_647",
	"S_648",
	"S_649",
	"S_650",
	"S_651",
	"S_652",
	"S_653",
	"S_654",
	"S_655",
	"S_656",
	"S_657",
	"S_658",
	"S_659",
	"S_660",
	"S_661",
	"S_662",
	"S_663",
	"S_664",
	"S_665",
	"S_666",
	"S_667",
	"S_668",
	"S_669",
	"S_670",
	"S_671",
	"S_672",
	"S_673",
	"S_674",
	"S_675",
	"S_676",
	"S_677",
	"S_678",
	"S_679",
	"S_680",
	"S_681",
	"S_682",
	"S_683",
	"S_684",
	"S_685",
	"S_686",
	"S_687",
	"S_688",
	"S_689",
	"S_690",
	"S_691",
	"S_692",
	"S_693",
	"S_694",
	"S_695",
	"S_696",
	"S_697",
	"S_698",
	"S_699",
	"S_700",
	"S_701",
	"S_702",
	"S_703",
	"S_704",
	"S_705",
	"S_706",
	"S_707",
	"S_708",
	"S_709",
	"S_710",
	"S_711",
	"S_712",
	"S_713",
	"S_714",
	"S_715",
	"S_716",
	"S_717",
	"S_718",
	"S_719",
	"S_720",
	"S_721",
	"S_722",
	"S_723",
	"S_724",
	"S_725",
	"S_726",
	"S_727",
	"S_728",
	"S_729",
	"S_730",
	"S_731",
	"S_732",
	"S_733",
	"S_734",
	"S_735",
	"S_736",
	"S_737",
	"S_738",
	"S_739",
	"S_740",
	"S_741",
	"S_742",
	"S_743",
	"S_744",
	"S_745",
	"S_746",
	"S_747",
	"S_748",
	"S_749",
	"S_750",
	"S_751",
	"S_752",
	"S_753",
	"S_754",
	"S_755",
	"S_756",
	"S_757",
	"S_758",
	"S_759",
	"S_760",
	"S_761",
	"S_762",
	"S_763",
	"S_764",
	"S_765",
	"S_766",
	"S_767",
	"S_768",
	"S_769",
	"S_770",
	"S_771",
	"S_772",
	"S_773",
	"S_774",
	"S_775",
	"S_776",
	"S_777",
	"S_778",
	"S_779",
	"S_780",
	"S_781",
	"S_782",
	"S_783",
	"S_784",
	"S_785",
	"S_786",
	"S_787",
	"S_788",
	"S_789",
	"S_790",
	"S_791",
	"S_792",
	"S_793",
	"S_794",
	"S_795",
	"S_796",
	"S_797",
	"S_798",
	"S_799",
	"S_800",
	"S_801",
	"S_802",
	"S_803",
	"S_804",
	"S_805",
	"S_806",
	"S_807",
	"S_808",
	"S_809",
	"S_810",
	"S_811",
	"S_812",
	"S_813",
	"S_814",
	"S_815",
	"S_816",
	"S_817",
	"S_818",
	"S_819",
	"S_820",
	"S_821",
	"S_822",
	"S_823",
	"S_824",
	"S_825",
	"S_826",
	"S_827",
	"S_828",
	"S_829",
	"S_830",
	"S_831",
	"S_832",
	"S_833",
	"S_834",
	"S_835",
	"S_836",
	"S_837",
	"S_838",
	"S_839",
	"S_840",
	"S_841",
	"S_842",
	"S_843",
	"S_844",
	"S_845",
	"S_846",
	"S_847",
	"S_848",
	"S_849",
	"S_850",
	"S_851",
	"S_852",
	"S_853",
	"S_854",
	"S_855",
	"S_856",
	"S_857",
	"S_858",
	"S_859",
	"S_860",
	"S_861",
	"S_862",
	"S_863",
	"S_864",
	"S_865",
	"S_866",
	"S_867",
	"S_868",
	"S_869",
	"S_870",
	"S_871",
	"S_872",
	"S_873",
	"S_874",
	"S_875",
	"S_876",
	"S_877",
	"S_878",
	"S_879",
	"S_880",
	"S_881",
	"S_882",
	"S_883",
	"S_884",
	"S_885",
	"S_886",
	"S_887",
	"S_888",
	"S_889",
	"S_890",
	"S_891",
	"S_892",
	"S_893",
	"S_894",
	"S_895",
	"S_896",
	"S_897",
	"S_898",
	"S_899",
	"S_900",
	"S_901",
	"S_902",
	"S_903",
	"S_904",
	"S_905",
	"S_906",
	"S_907",
	"S_908",
	"S_909",
	"S_910",
	"S_911",
	"S_912",
	"S_913",
	"S_914",
	"S_915",
	"S_916",
	"S_917",
	"S_918",
	"S_919",
	"S_920",
	"S_921",
	"S_922",
	"S_923",
	"S_924",
	"S_925",
	"S_926",
	"S_927",
	"S_928",
	"S_929",
	"S_930",
	"S_931",
	"S_932",
	"S_933",
	"S_934",
	"S_935",
	"S_936",
	"S_937",
	"S_938",
	"S_939",
	"S_940",
	"S_941",
	"S_942",
	"S_943",
	"S_944",
	"S_945",
	"S_946",
	"S_947",
	"S_948",
	"S_949",
	"S_950",
	"S_951",
	"S_952",
	"S_953",
	"S_954",
	"S_955",
	"S_956",
	"S_957",
	"S_958",
	"S_959",
	"S_960",
	"S_961",
	"S_962",
	"S_963",
	"S_964",
	"S_965",
	"S_966",
	"S_967",
	"S_968",
	"S_969",
	"S_970",
	"S_971",
	"S_972",
	"S_973",
	"S_974",
	"S_975",
	"S_976",
	"S_977",
	"S_978",
	"S_979",
	"S_980",
	"S_981",
	"S_982",
	"S_983",
	"S_984",
	"S_985",
	"S_986",
	"S_987",
	"S_988",
	"S_989",
	"S_990",
	"S_991",
	"S_992",
	"S_993",
	"S_994",
	"S_995",
	"S_996",
	"S_997",
	"S_998",
	"S_999",
	"S_1000",
	"S_1001",
	"S_1002",
	"S_1003",
	"S_1004",
	"S_1005",
	"S_1006",
	"S_1007",
	"S_1008",
	"S_1009",
	"S_1010",
	"S_1011",
	"S_1012",
	"S_1013",
	"S_1014",
	"S_1015",
	"S_1016",
	"S_1017",
	"S_1018",
	"S_1019",
	"S_1020",
	"S_1021",
	"S_1022",
	"S_1023",
	"S_1024",
	"S_1025",
	"S_1026",
	"S_1027",
	"S_1028",
	"S_1029",
	"S_1030",
	"S_1031",
	"S_1032",
	"S_1033",
	"S_1034",
	"S_1035",
	"S_1036",
	"S_1037",
	"S_1038",
	"S_1039",
	"S_1040",
	"S_1041",
	"S_1042",
	"S_1043",
	"S_1044",
	"S_1045",
	"S_1046",
	"S_1047",
	"S_1048",
	"S_1049",
	"S_1050",
	"S_1051",
	"S_1052",
	"S_1053",
	"S_1054",
	"S_1055",
	"S_1056",
	"S_1057",
	"S_1058",
	"S_1059",
	"S_1060",
	"S_1061",
	"S_1062",
	"S_1063",
	"S_1064",
	"S_1065",
	"S_1066",
	"S_1067",
	"S_1068",
	"S_1069",
	"S_1070",
	"S_1071",
	"S_1072",
	"S_1073",
	"S_1074",
	"S_1075",
	"S_1076",
	"S_1077",
	"S_1078",
	"S_1079",
	"S_1080",
	"S_1081",
	"S_1082",
	"S_1083",
	"S_1084",
	"S_1085",
	"S_1086",
	"S_1087",
	"S_1088",
	"S_1089",
	"S_1090",
	"S_1091",
	"S_1092",
	"S_1093",
	"S_1094",
	"S_1095",
	"S_1096",
	"S_1097",
	"S_1098",
	"S_1099",
	"S_1100",
	"S_1101",
	"S_1102",
	"S_1103",
	"S_1104",
	"S_1105",
	"S_1106",
	"S_1107",
	"S_1108",
	"S_1109",
	"S_1110",
	"S_1111",
	"S_1112",
	"S_1113",
	"S_1114",
	"S_1115",
	"S_1116",
	"S_1117",
	"S_1118",
	"S_1119",
	"S_1120",
	"S_1121",
	"S_1122",
	"S_1123",
	"S_1124",
	"S_1125",
	"S_1126",
	"S_1127",
	"S_1128",
	"S_1129",
	"S_1130",
	"S_1131",
	"S_1132",
	"S_1133",
	"S_1134",
	"S_1135",
	"S_1136",
	"S_1137",
	"S_1138",
	"S_1139",
	"S_1140",
	"S_1141",
	"S_1142",
	"S_1143",
	"S_1144",
	"S_1145",
	"S_1146",
	"S_1147",
	"S_1148",
	"S_1149",
	"S_1150",
	"S_1151",
	"S_1152",
	"S_1153",
	"S_1154",
	"S_1155",
	"S_1156",
	"S_1157",
	"S_1158",
	"S_1159",
	"S_1160",
	"S_1161",
	"S_1162",
	"S_1163",
	"S_1164",
	"S_1165",
	"S_1166",
	"S_1167",
	"S_1168",
	"S_1169",
	"S_1170",
	"S_1171",
	"S_1172",
	"S_1173",
	"S_1174",
	"S_1175",
	"S_1176",
	"S_1177",
	"S_1178",
	"S_1179",
	"S_1180",
	"S_1181",
	"S_1182",
	"S_1183",
	"S_1184",
	"S_1185",
	"S_1186",
	"S_1187",
	"S_1188",
	"S_1189",
	"S_1190",
	"S_1191",
	"S_1192",
	"S_1193",
	"S_1194",
	"S_1195",
	"S_1196",
	"S_1197",
	"S_1198",
	"S_1199",
	"S_1200",
	"S_1201",
	"S_1202",
	"S_1203",
	"S_1204",
	"S_1205",
	"S_1206",
	"S_1207",
	"S_1208",
	"S_1209",
	"S_1210",
	"S_1211",
	"S_1212",
	"S_1213",
	"S_1214",
	"S_1215",
	"S_1216",
	"S_1217",
	"S_1218",
	"S_1219",
	"S_1220",
	"S_1221",
	"S_1222",
	"S_1223",
	"S_1224",
	"S_1225",
	"S_1226",
	"S_1227",
	"S_1228",
	"S_1229",
	"S_1230",
	"S_1231",
	"S_1232",
	"S_1233",
	"S_1234",
	"S_1235",
	"S_1236",
	"S_1237",
	"S_1238",
	"S_1239",
	"S_1240",
	"S_1241",
	"S_1242",
	"S_1243",
	"S_1244",
	"S_1245",
	"S_1246",
	"S_1247",
	"S_1248",
	"S_1249",
	"S_1250",
	"S_1251",
	"S_1252",
	"S_1253",
	"S_1254",
	"S_1255",
	"S_1256",
	"S_1257",
	"S_1258",
	"S_1259",
	"S_1260",
	"S_1261",
	"S_1262",
	"S_1263",
	"S_1264",
	"S_1265",
	"S_1266",
	"S_1267",
	"S_1268",
	"S_1269",
	"S_1270",
	"S_1271",
	"S_1272",
	"S_1273",
	"S_1274",
	"S_1275",
	"S_1276",
	"S_1277",
	"S_1278",
	"S_1279",
	"S_1280",
	"S_1281",
	"S_1282",
	"S_1283",
	"S_1284",
	"S_1285",
	"S_1286",
	"S_1287",
	"S_1288",
	"S_1289",
	"S_1290",
	"S_1291",
	"S_1292",
	"S_1293",
	"S_1294",
	"S_1295",
	"S_1296",
	"S_1297",
	"S_1298",
	"S_1299",
	"S_1300",
	"S_1301",
	"S_1302",
	"S_1303",
	"S_1304",
	"S_1305",
	"S_1306",
	"S_1307",
	"S_1308",
	"S_1309",
	"S_1310",
	"S_1311",
	"S_1312",
	"S_1313",
	"S_1314",
	"S_1315",
	"S_1316",
	"S_1317",
	"S_1318",
	"S_1319",
	"S_1320",
	"S_1321",
	"S_1322",
	"S_1323",
	"S_1324",
	"S_1325",
	"S_1326",
	"S_1327",
	"S_1328",
	"S_1329",
	"S_1330",
	"S_1331",
	"S_1332",
	"S_1333",
	"S_1334",
	"S_1335",
	"S_1336",
	"S_1337",
	"S_1338",
	"S_1339",
	"S_1340",
	"S_1341",
	"S_1342",
	"S_1343",
	"S_1344",
	"S_1345",
	"S_1346",
	"S_1347",
	"S_1348",
	"S_1349",
	"S_1350",
	"S_1351",
	"S_1352",
	"S_1353",
	"S_1354",
	"S_1355",
	"S_1356",
	"S_1357",
	"S_1358",
	"S_1359",
	"S_1360",
	"S_1361",
	"S_1362",
	"S_1363",
	"S_1364",
	"S_1365",
	"S_1366",
	"S_1367",
	"S_1368",
	"S_1369",
	"S_1370",
	"S_1371",
	"S_1372",
	"S_1373",
	"S_1374",
	"S_1375",
	"S_1376",
	"S_1377",
	"S_1378",
	"S_1379",
	"S_1380",
	"S_1381",
	"S_1382",
	"S_1383",
	"S_1384",
	"S_1385",
	"S_1386",
	"S_1387",
	"S_1388",
	"S_1389",
	"S_1390",
	"S_1391",
	"S_1392",
	"S_1393",
	"S_1394",
	"S_1395",
	"S_1396",
	"S_1397",
	"S_1398",
	"S_1399",
	"S_1400",
	"S_1401",
	"S_1402",
	"S_1403",
	"S_1404",
	"S_1405",
	"S_1406",
	"S_1407",
	"S_1408",
	"S_1409",
	"S_1410",
	"S_1411",
	"S_1412",
	"S_1413",
	"S_1414",
	"S_1415",
	"S_1416",
	"S_1417",
	"S_1418",
	"S_1419",
	"S_1420",
	"S_1421",
	"S_1422",
	"S_1423",
	"S_1424",
	"S_1425",
	"S_1426",
	"S_1427",
	"S_1428",
	"S_1429",
	"S_1430",
	"S_1431",
	"S_1432",
	"S_1433",
	"S_1434",
	"S_1435",
	"S_1436",
	"S_1437",
	"S_1438",
	"S_1439",
	"S_1440",
	"S_1441",
	"S_1442",
	"S_1443",
	"S_1444",
	"S_1445",
	"S_1446",
	"S_1447",
	"S_1448",
	"S_1449",
	"S_1450",
	"S_1451",
	"S_1452",
	"S_1453",
	"S_1454",
	"S_1455",
	"S_1456",
	"S_1457",
	"S_1458",
	"S_1459",
	"S_1460",
	"S_1461",
	"S_1462",
	"S_1463",
	"S_1464",
	"S_1465",
	"S_1466",
	"S_1467",
	"S_1468",
	"S_1469",
	"S_1470",
	"S_1471",
	"S_1472",
	"S_1473",
	"S_1474",
	"S_1475",
	"S_1476",
	"S_1477",
	"S_1478",
	"S_1479",
	"S_1480",
	"S_1481",
	"S_1482",
	"S_1483",
	"S_1484",
	"S_1485",
	"S_1486",
	"S_1487",
	"S_1488",
	"S_1489",
	"S_1490",
	"S_1491",
	"S_1492",
	"S_1493",
	"S_1494",
	"S_1495",
	"S_1496",
	"S_1497",
	"S_1498",
	"S_1499",
	"S_1500",
	"S_1501",
	"S_1502",
	"S_1503",
	"S_1504",
	"S_1505",
	"S_1506",
	"S_1507",
	"S_1508",
	"S_1509",
	"S_1510",
	"S_1511",
	"S_1512",
	"S_1513",
	"S_1514",
	"S_1515",
	"S_1516",
	"S_1517",
	"S_1518",
	"S_1519",
	"S_1520",
	"S_1521",
	"S_1522",
	"S_1523",
	"S_1524",
	"S_1525",
	"S_1526",
	"S_1527",
	"S_1528",
	"S_1529",
	"S_1530",
	"S_1531",
	"S_1532",
	"S_1533",
	"S_1534",
	"S_1535",
	"S_1536",
	"S_1537",
	"S_1538",
	"S_1539",
	"S_1540",
	"S_1541",
	"S_1542",
	"S_1543",
	"S_1544",
	"S_1545",
	"S_1546",
	"S_1547",
	"S_1548",
	"S_1549",
	"S_1550",
	"S_1551",
	"S_1552",
	"S_1553",
	"S_1554",
	"S_1555",
	"S_1556",
	"S_1557",
	"S_1558",
	"S_1559",
	"S_1560",
	"S_1561",
	"S_1562",
	"S_1563",
	"S_1564",
};

//**************************************************************************
//
//	$Log$
//	Revision 1.4  2002/01/07 12:31:35  dj_jl
//	Changed copyright year
//
//	Revision 1.3  2001/09/20 16:35:58  dj_jl
//	Beautification
//	
//	Revision 1.2  2001/07/27 14:27:56  dj_jl
//	Update with Id-s and Log-s, some fixes
//
//**************************************************************************
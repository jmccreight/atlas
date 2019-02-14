/*
 * (C) Copyright 2013 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 * In applying this licence, ECMWF does not waive the privileges and immunities
 * granted to it by virtue of its status as an intergovernmental organisation
 * nor does it submit to any jurisdiction.
 */

// TL3199

#include "atlas/grid/detail/spacing/gaussian/N.h"

namespace atlas {
namespace grid {
namespace spacing {
namespace gaussian {

DEFINE_GAUSSIAN_LATITUDES(
    1600,
    LIST(
        89.956948491058, 89.901178822991, 89.845079804890, 89.788906372571, 89.732704713178, 89.676489394638,
        89.620266442582, 89.564038795892, 89.507808057494, 89.451575175583, 89.395340746771, 89.339105165153,
        89.282868701480, 89.226631547902, 89.170393844551, 89.114155696023, 89.057917181969, 89.001678364113,
        88.945439291023, 88.889200001442, 88.832960526648, 88.776720892172, 88.720481119062, 88.664241224818,
        88.608001224118, 88.551761129360, 88.495520951086, 88.439280698324, 88.383040378843, 88.326799999369,
        88.270559565753, 88.214319083110, 88.158078555930, 88.101837988170, 88.045597383333, 87.989356744529,
        87.933116074532, 87.876875375818, 87.820634650612, 87.764393900912, 87.708153128521, 87.651912335070,
        87.595671522036, 87.539430690761, 87.483189842469, 87.426948978275, 87.370708099200, 87.314467206178,
        87.258226300067, 87.201985381657, 87.145744451675, 87.089503510791, 87.033262559625, 86.977021598752,
        86.920780628702, 86.864539649971, 86.808298663016, 86.752057668265, 86.695816666116, 86.639575656940,
        86.583334641085, 86.527093618874, 86.470852590613, 86.414611556584, 86.358370517056, 86.302129472280,
        86.245888422491, 86.189647367911, 86.133406308749, 86.077165245203, 86.020924177459, 85.964683105691,
        85.908442030065, 85.852200950740, 85.795959867862, 85.739718781574, 85.683477692007, 85.627236599289,
        85.570995503540, 85.514754404873, 85.458513303398, 85.402272199216, 85.346031092427, 85.289789983123,
        85.233548871394, 85.177307757325, 85.121066640996, 85.064825522485, 85.008584401865, 84.952343279207,
        84.896102154578, 84.839861028043, 84.783619899663, 84.727378769498, 84.671137637604, 84.614896504035,
        84.558655368843, 84.502414232077, 84.446173093787, 84.389931954017, 84.333690812811, 84.277449670213,
        84.221208526262, 84.164967380998, 84.108726234457, 84.052485086678, 83.996243937694, 83.940002787538,
        83.883761636244, 83.827520483842, 83.771279330362, 83.715038175834, 83.658797020285, 83.602555863742,
        83.546314706230, 83.490073547776, 83.433832388404, 83.377591228136, 83.321350066995, 83.265108905004,
        83.208867742183, 83.152626578553, 83.096385414133, 83.040144248943, 82.983903083002, 82.927661916327,
        82.871420748935, 82.815179580843, 82.758938412069, 82.702697242626, 82.646456072532, 82.590214901800,
        82.533973730445, 82.477732558482, 82.421491385923, 82.365250212781, 82.309009039070, 82.252767864802,
        82.196526689989, 82.140285514643, 82.084044338775, 82.027803162396, 81.971561985516, 81.915320808147,
        81.859079630299, 81.802838451980, 81.746597273202, 81.690356093972, 81.634114914301, 81.577873734197,
        81.521632553669, 81.465391372725, 81.409150191374, 81.352909009622, 81.296667827479, 81.240426644952,
        81.184185462047, 81.127944278772, 81.071703095135, 81.015461911141, 80.959220726798, 80.902979542112,
        80.846738357090, 80.790497171737, 80.734255986059, 80.678014800063, 80.621773613754, 80.565532427138,
        80.509291240220, 80.453050053006, 80.396808865500, 80.340567677708, 80.284326489635, 80.228085301286,
        80.171844112665, 80.115602923777, 80.059361734627, 80.003120545219, 79.946879355557, 79.890638165646,
        79.834396975489, 79.778155785091, 79.721914594456, 79.665673403587, 79.609432212489, 79.553191021165,
        79.496949829619, 79.440708637854, 79.384467445874, 79.328226253682, 79.271985061281, 79.215743868675,
        79.159502675867, 79.103261482861, 79.047020289658, 78.990779096262, 78.934537902677, 78.878296708904,
        78.822055514948, 78.765814320810, 78.709573126493, 78.653331932000, 78.597090737333, 78.540849542496,
        78.484608347490, 78.428367152319, 78.372125956984, 78.315884761487, 78.259643565832, 78.203402370020,
        78.147161174054, 78.090919977936, 78.034678781667, 77.978437585251, 77.922196388689, 77.865955191983,
        77.809713995135, 77.753472798147, 77.697231601021, 77.640990403760, 77.584749206363, 77.528508008835,
        77.472266811176, 77.416025613388, 77.359784415473, 77.303543217432, 77.247302019268, 77.191060820981,
        77.134819622574, 77.078578424048, 77.022337225405, 76.966096026645, 76.909854827771, 76.853613628784,
        76.797372429686, 76.741131230477, 76.684890031160, 76.628648831736, 76.572407632205, 76.516166432570,
        76.459925232831, 76.403684032991, 76.347442833049, 76.291201633008, 76.234960432869, 76.178719232632,
        76.122478032300, 76.066236831873, 76.009995631352, 75.953754430738, 75.897513230033, 75.841272029238,
        75.785030828353, 75.728789627381, 75.672548426321, 75.616307225175, 75.560066023944, 75.503824822628,
        75.447583621230, 75.391342419749, 75.335101218187, 75.278860016545, 75.222618814823, 75.166377613023,
        75.110136411145, 75.053895209190, 74.997654007160, 74.941412805054, 74.885171602875, 74.828930400621,
        74.772689198296, 74.716447995898, 74.660206793430, 74.603965590891, 74.547724388284, 74.491483185607,
        74.435241982862, 74.379000780051, 74.322759577172, 74.266518374228, 74.210277171219, 74.154035968146,
        74.097794765009, 74.041553561809, 73.985312358547, 73.929071155223, 73.872829951838, 73.816588748392,
        73.760347544887, 73.704106341323, 73.647865137700, 73.591623934019, 73.535382730281, 73.479141526486,
        73.422900322635, 73.366659118728, 73.310417914766, 73.254176710750, 73.197935506680, 73.141694302556,
        73.085453098380, 73.029211894151, 72.972970689870, 72.916729485538, 72.860488281156, 72.804247076723,
        72.748005872240, 72.691764667707, 72.635523463126, 72.579282258497, 72.523041053820, 72.466799849095,
        72.410558644323, 72.354317439505, 72.298076234640, 72.241835029730, 72.185593824775, 72.129352619775,
        72.073111414731, 72.016870209642, 71.960629004510, 71.904387799335, 71.848146594117, 71.791905388857,
        71.735664183555, 71.679422978211, 71.623181772826, 71.566940567400, 71.510699361934, 71.454458156428,
        71.398216950882, 71.341975745297, 71.285734539673, 71.229493334010, 71.173252128309, 71.117010922570,
        71.060769716793, 71.004528510979, 70.948287305128, 70.892046099240, 70.835804893316, 70.779563687357,
        70.723322481361, 70.667081275330, 70.610840069264, 70.554598863164, 70.498357657029, 70.442116450859,
        70.385875244656, 70.329634038420, 70.273392832150, 70.217151625847, 70.160910419512, 70.104669213144,
        70.048428006743, 69.992186800311, 69.935945593848, 69.879704387353, 69.823463180827, 69.767221974270,
        69.710980767683, 69.654739561065, 69.598498354417, 69.542257147739, 69.486015941032, 69.429774734296,
        69.373533527530, 69.317292320736, 69.261051113913, 69.204809907061, 69.148568700182, 69.092327493274,
        69.036086286339, 68.979845079376, 68.923603872387, 68.867362665370, 68.811121458326, 68.754880251256,
        68.698639044159, 68.642397837036, 68.586156629887, 68.529915422712, 68.473674215512, 68.417433008286,
        68.361191801036, 68.304950593760, 68.248709386459, 68.192468179134, 68.136226971784, 68.079985764411,
        68.023744557013, 67.967503349591, 67.911262142146, 67.855020934677, 67.798779727185, 67.742538519670,
        67.686297312132, 67.630056104571, 67.573814896987, 67.517573689381, 67.461332481753, 67.405091274103,
        67.348850066431, 67.292608858736, 67.236367651021, 67.180126443284, 67.123885235525, 67.067644027746,
        67.011402819945, 66.955161612124, 66.898920404282, 66.842679196420, 66.786437988537, 66.730196780634,
        66.673955572711, 66.617714364767, 66.561473156805, 66.505231948822, 66.448990740820, 66.392749532799,
        66.336508324758, 66.280267116698, 66.224025908620, 66.167784700522, 66.111543492406, 66.055302284271,
        65.999061076118, 65.942819867947, 65.886578659757, 65.830337451550, 65.774096243324, 65.717855035081,
        65.661613826820, 65.605372618542, 65.549131410246, 65.492890201932, 65.436648993602, 65.380407785255,
        65.324166576890, 65.267925368509, 65.211684160111, 65.155442951697, 65.099201743266, 65.042960534818,
        64.986719326355, 64.930478117875, 64.874236909379, 64.817995700867, 64.761754492340, 64.705513283796,
        64.649272075237, 64.593030866663, 64.536789658073, 64.480548449468, 64.424307240847, 64.368066032212,
        64.311824823562, 64.255583614896, 64.199342406216, 64.143101197521, 64.086859988811, 64.030618780087,
        63.974377571349, 63.918136362596, 63.861895153829, 63.805653945048, 63.749412736253, 63.693171527443,
        63.636930318620, 63.580689109783, 63.524447900933, 63.468206692069, 63.411965483191, 63.355724274300,
        63.299483065395, 63.243241856477, 63.187000647546, 63.130759438602, 63.074518229645, 63.018277020675,
        62.962035811692, 62.905794602697, 62.849553393688, 62.793312184667, 62.737070975634, 62.680829766588,
        62.624588557529, 62.568347348459, 62.512106139376, 62.455864930281, 62.399623721174, 62.343382512055,
        62.287141302924, 62.230900093781, 62.174658884626, 62.118417675459, 62.062176466281, 62.005935257092,
        61.949694047891, 61.893452838678, 61.837211629454, 61.780970420219, 61.724729210972, 61.668488001715,
        61.612246792446, 61.556005583166, 61.499764373875, 61.443523164574, 61.387281955261, 61.331040745938,
        61.274799536604, 61.218558327259, 61.162317117904, 61.106075908538, 61.049834699162, 60.993593489776,
        60.937352280379, 60.881111070972, 60.824869861554, 60.768628652127, 60.712387442689, 60.656146233241,
        60.599905023784, 60.543663814316, 60.487422604839, 60.431181395352, 60.374940185855, 60.318698976348,
        60.262457766832, 60.206216557306, 60.149975347770, 60.093734138225, 60.037492928671, 59.981251719107,
        59.925010509534, 59.868769299952, 59.812528090360, 59.756286880760, 59.700045671150, 59.643804461531,
        59.587563251903, 59.531322042266, 59.475080832621, 59.418839622966, 59.362598413303, 59.306357203630,
        59.250115993950, 59.193874784260, 59.137633574562, 59.081392364855, 59.025151155140, 58.968909945416,
        58.912668735684, 58.856427525944, 58.800186316195, 58.743945106438, 58.687703896672, 58.631462686899,
        58.575221477117, 58.518980267327, 58.462739057529, 58.406497847723, 58.350256637909, 58.294015428087,
        58.237774218258, 58.181533008420, 58.125291798575, 58.069050588721, 58.012809378861, 57.956568168992,
        57.900326959116, 57.844085749232, 57.787844539340, 57.731603329441, 57.675362119535, 57.619120909621,
        57.562879699700, 57.506638489771, 57.450397279835, 57.394156069892, 57.337914859941, 57.281673649983,
        57.225432440018, 57.169191230046, 57.112950020067, 57.056708810081, 57.000467600088, 56.944226390087,
        56.887985180080, 56.831743970066, 56.775502760045, 56.719261550017, 56.663020339982, 56.606779129941,
        56.550537919892, 56.494296709838, 56.438055499776, 56.381814289708, 56.325573079633, 56.269331869551,
        56.213090659463, 56.156849449369, 56.100608239268, 56.044367029160, 55.988125819046, 55.931884608926,
        55.875643398799, 55.819402188666, 55.763160978527, 55.706919768382, 55.650678558230, 55.594437348072,
        55.538196137908, 55.481954927738, 55.425713717562, 55.369472507379, 55.313231297191, 55.256990086997,
        55.200748876796, 55.144507666590, 55.088266456378, 55.032025246159, 54.975784035935, 54.919542825706,
        54.863301615470, 54.807060405229, 54.750819194981, 54.694577984728, 54.638336774470, 54.582095564206,
        54.525854353936, 54.469613143660, 54.413371933379, 54.357130723092, 54.300889512800, 54.244648302502,
        54.188407092199, 54.132165881890, 54.075924671576, 54.019683461257, 53.963442250932, 53.907201040602,
        53.850959830266, 53.794718619925, 53.738477409579, 53.682236199228, 53.625994988871, 53.569753778510,
        53.513512568143, 53.457271357770, 53.401030147393, 53.344788937011, 53.288547726623, 53.232306516231,
        53.176065305833, 53.119824095431, 53.063582885023, 53.007341674611, 52.951100464193, 52.894859253771,
        52.838618043344, 52.782376832912, 52.726135622475, 52.669894412033, 52.613653201586, 52.557411991135,
        52.501170780679, 52.444929570218, 52.388688359752, 52.332447149282, 52.276205938807, 52.219964728328,
        52.163723517843, 52.107482307354, 52.051241096861, 51.994999886363, 51.938758675860, 51.882517465353,
        51.826276254842, 51.770035044326, 51.713793833805, 51.657552623280, 51.601311412751, 51.545070202217,
        51.488828991679, 51.432587781136, 51.376346570589, 51.320105360038, 51.263864149482, 51.207622938923,
        51.151381728358, 51.095140517790, 51.038899307217, 50.982658096641, 50.926416886060, 50.870175675474,
        50.813934464885, 50.757693254292, 50.701452043694, 50.645210833092, 50.588969622486, 50.532728411876,
        50.476487201263, 50.420245990645, 50.364004780023, 50.307763569397, 50.251522358767, 50.195281148133,
        50.139039937495, 50.082798726853, 50.026557516207, 49.970316305558, 49.914075094904, 49.857833884247,
        49.801592673586, 49.745351462921, 49.689110252252, 49.632869041579, 49.576627830903, 49.520386620223,
        49.464145409539, 49.407904198851, 49.351662988160, 49.295421777465, 49.239180566766, 49.182939356064,
        49.126698145358, 49.070456934648, 49.014215723935, 48.957974513218, 48.901733302498, 48.845492091774,
        48.789250881046, 48.733009670315, 48.676768459580, 48.620527248842, 48.564286038100, 48.508044827355,
        48.451803616606, 48.395562405854, 48.339321195099, 48.283079984340, 48.226838773577, 48.170597562811,
        48.114356352042, 48.058115141270, 48.001873930494, 47.945632719714, 47.889391508932, 47.833150298146,
        47.776909087357, 47.720667876564, 47.664426665768, 47.608185454969, 47.551944244167, 47.495703033362,
        47.439461822553, 47.383220611741, 47.326979400926, 47.270738190108, 47.214496979286, 47.158255768461,
        47.102014557634, 47.045773346803, 46.989532135969, 46.933290925132, 46.877049714291, 46.820808503448,
        46.764567292602, 46.708326081752, 46.652084870900, 46.595843660044, 46.539602449186, 46.483361238324,
        46.427120027460, 46.370878816592, 46.314637605722, 46.258396394849, 46.202155183972, 46.145913973093,
        46.089672762211, 46.033431551326, 45.977190340438, 45.920949129547, 45.864707918653, 45.808466707756,
        45.752225496857, 45.695984285954, 45.639743075049, 45.583501864141, 45.527260653230, 45.471019442317,
        45.414778231400, 45.358537020481, 45.302295809559, 45.246054598635, 45.189813387707, 45.133572176777,
        45.077330965844, 45.021089754909, 44.964848543971, 44.908607333030, 44.852366122086, 44.796124911140,
        44.739883700191, 44.683642489239, 44.627401278285, 44.571160067328, 44.514918856368, 44.458677645406,
        44.402436434442, 44.346195223474, 44.289954012505, 44.233712801532, 44.177471590557, 44.121230379580,
        44.064989168600, 44.008747957617, 43.952506746632, 43.896265535644, 43.840024324654, 43.783783113662,
        43.727541902667, 43.671300691669, 43.615059480669, 43.558818269667, 43.502577058662, 43.446335847655,
        43.390094636645, 43.333853425633, 43.277612214618, 43.221371003601, 43.165129792582, 43.108888581560,
        43.052647370536, 42.996406159510, 42.940164948481, 42.883923737450, 42.827682526417, 42.771441315381,
        42.715200104343, 42.658958893302, 42.602717682260, 42.546476471215, 42.490235260168, 42.433994049118,
        42.377752838066, 42.321511627012, 42.265270415956, 42.209029204898, 42.152787993837, 42.096546782774,
        42.040305571709, 41.984064360641, 41.927823149572, 41.871581938500, 41.815340727426, 41.759099516350,
        41.702858305272, 41.646617094191, 41.590375883109, 41.534134672024, 41.477893460937, 41.421652249848,
        41.365411038757, 41.309169827664, 41.252928616569, 41.196687405472, 41.140446194372, 41.084204983271,
        41.027963772167, 40.971722561062, 40.915481349954, 40.859240138844, 40.802998927732, 40.746757716619,
        40.690516505503, 40.634275294385, 40.578034083265, 40.521792872143, 40.465551661019, 40.409310449894,
        40.353069238766, 40.296828027636, 40.240586816504, 40.184345605371, 40.128104394235, 40.071863183097,
        40.015621971958, 39.959380760816, 39.903139549673, 39.846898338528, 39.790657127381, 39.734415916231,
        39.678174705080, 39.621933493928, 39.565692282773, 39.509451071616, 39.453209860458, 39.396968649297,
        39.340727438135, 39.284486226971, 39.228245015805, 39.172003804637, 39.115762593468, 39.059521382297,
        39.003280171123, 38.947038959948, 38.890797748772, 38.834556537593, 38.778315326413, 38.722074115230,
        38.665832904047, 38.609591692861, 38.553350481673, 38.497109270484, 38.440868059293, 38.384626848100,
        38.328385636906, 38.272144425710, 38.215903214512, 38.159662003312, 38.103420792111, 38.047179580908,
        37.990938369703, 37.934697158497, 37.878455947289, 37.822214736079, 37.765973524867, 37.709732313654,
        37.653491102439, 37.597249891223, 37.541008680005, 37.484767468785, 37.428526257564, 37.372285046341,
        37.316043835116, 37.259802623890, 37.203561412662, 37.147320201432, 37.091078990201, 37.034837778968,
        36.978596567734, 36.922355356498, 36.866114145260, 36.809872934021, 36.753631722781, 36.697390511538,
        36.641149300295, 36.584908089049, 36.528666877802, 36.472425666554, 36.416184455304, 36.359943244052,
        36.303702032799, 36.247460821545, 36.191219610289, 36.134978399031, 36.078737187772, 36.022495976511,
        35.966254765249, 35.910013553986, 35.853772342721, 35.797531131454, 35.741289920186, 35.685048708916,
        35.628807497645, 35.572566286373, 35.516325075099, 35.460083863824, 35.403842652547, 35.347601441269,
        35.291360229989, 35.235119018708, 35.178877807425, 35.122636596141, 35.066395384856, 35.010154173569,
        34.953912962281, 34.897671750991, 34.841430539700, 34.785189328408, 34.728948117114, 34.672706905819,
        34.616465694522, 34.560224483224, 34.503983271925, 34.447742060625, 34.391500849323, 34.335259638019,
        34.279018426714, 34.222777215408, 34.166536004101, 34.110294792792, 34.054053581482, 33.997812370171,
        33.941571158858, 33.885329947544, 33.829088736229, 33.772847524912, 33.716606313594, 33.660365102275,
        33.604123890954, 33.547882679632, 33.491641468309, 33.435400256985, 33.379159045659, 33.322917834332,
        33.266676623004, 33.210435411675, 33.154194200344, 33.097952989012, 33.041711777679, 32.985470566344,
        32.929229355008, 32.872988143671, 32.816746932333, 32.760505720994, 32.704264509653, 32.648023298311,
        32.591782086968, 32.535540875624, 32.479299664278, 32.423058452931, 32.366817241583, 32.310576030234,
        32.254334818884, 32.198093607532, 32.141852396180, 32.085611184826, 32.029369973471, 31.973128762114,
        31.916887550757, 31.860646339398, 31.804405128039, 31.748163916678, 31.691922705316, 31.635681493953,
        31.579440282588, 31.523199071223, 31.466957859856, 31.410716648488, 31.354475437119, 31.298234225749,
        31.241993014378, 31.185751803006, 31.129510591633, 31.073269380258, 31.017028168883, 30.960786957506,
        30.904545746128, 30.848304534749, 30.792063323369, 30.735822111988, 30.679580900606, 30.623339689223,
        30.567098477839, 30.510857266453, 30.454616055067, 30.398374843679, 30.342133632291, 30.285892420901,
        30.229651209510, 30.173409998119, 30.117168786726, 30.060927575332, 30.004686363937, 29.948445152541,
        29.892203941144, 29.835962729746, 29.779721518347, 29.723480306947, 29.667239095546, 29.610997884144,
        29.554756672741, 29.498515461337, 29.442274249932, 29.386033038526, 29.329791827119, 29.273550615711,
        29.217309404302, 29.161068192891, 29.104826981480, 29.048585770068, 28.992344558655, 28.936103347241,
        28.879862135826, 28.823620924410, 28.767379712994, 28.711138501576, 28.654897290157, 28.598656078737,
        28.542414867316, 28.486173655895, 28.429932444472, 28.373691233049, 28.317450021624, 28.261208810199,
        28.204967598772, 28.148726387345, 28.092485175917, 28.036243964487, 27.980002753057, 27.923761541626,
        27.867520330194, 27.811279118762, 27.755037907328, 27.698796695893, 27.642555484458, 27.586314273021,
        27.530073061584, 27.473831850146, 27.417590638707, 27.361349427267, 27.305108215826, 27.248867004384,
        27.192625792941, 27.136384581498, 27.080143370053, 27.023902158608, 26.967660947162, 26.911419735715,
        26.855178524267, 26.798937312818, 26.742696101369, 26.686454889918, 26.630213678467, 26.573972467015,
        26.517731255562, 26.461490044108, 26.405248832653, 26.349007621198, 26.292766409742, 26.236525198285,
        26.180283986827, 26.124042775368, 26.067801563908, 26.011560352448, 25.955319140987, 25.899077929525,
        25.842836718062, 25.786595506598, 25.730354295134, 25.674113083668, 25.617871872202, 25.561630660735,
        25.505389449268, 25.449148237799, 25.392907026330, 25.336665814860, 25.280424603389, 25.224183391918,
        25.167942180446, 25.111700968972, 25.055459757499, 24.999218546024, 24.942977334548, 24.886736123072,
        24.830494911595, 24.774253700118, 24.718012488639, 24.661771277160, 24.605530065680, 24.549288854199,
        24.493047642718, 24.436806431236, 24.380565219753, 24.324324008269, 24.268082796785, 24.211841585300,
        24.155600373814, 24.099359162327, 24.043117950840, 23.986876739352, 23.930635527863, 23.874394316374,
        23.818153104884, 23.761911893393, 23.705670681901, 23.649429470409, 23.593188258916, 23.536947047422,
        23.480705835928, 23.424464624433, 23.368223412937, 23.311982201441, 23.255740989943, 23.199499778446,
        23.143258566947, 23.087017355448, 23.030776143948, 22.974534932447, 22.918293720946, 22.862052509444,
        22.805811297942, 22.749570086438, 22.693328874935, 22.637087663430, 22.580846451925, 22.524605240419,
        22.468364028912, 22.412122817405, 22.355881605897, 22.299640394389, 22.243399182880, 22.187157971370,
        22.130916759859, 22.074675548348, 22.018434336837, 21.962193125324, 21.905951913811, 21.849710702298,
        21.793469490784, 21.737228279269, 21.680987067753, 21.624745856237, 21.568504644721, 21.512263433203,
        21.456022221685, 21.399781010167, 21.343539798648, 21.287298587128, 21.231057375607, 21.174816164087,
        21.118574952565, 21.062333741043, 21.006092529520, 20.949851317997, 20.893610106473, 20.837368894948,
        20.781127683423, 20.724886471897, 20.668645260371, 20.612404048844, 20.556162837317, 20.499921625789,
        20.443680414260, 20.387439202731, 20.331197991201, 20.274956779671, 20.218715568140, 20.162474356609,
        20.106233145077, 20.049991933544, 19.993750722011, 19.937509510477, 19.881268298943, 19.825027087408,
        19.768785875873, 19.712544664337, 19.656303452801, 19.600062241264, 19.543821029726, 19.487579818188,
        19.431338606650, 19.375097395110, 19.318856183571, 19.262614972031, 19.206373760490, 19.150132548949,
        19.093891337407, 19.037650125865, 18.981408914322, 18.925167702779, 18.868926491235, 18.812685279690,
        18.756444068146, 18.700202856600, 18.643961645054, 18.587720433508, 18.531479221961, 18.475238010414,
        18.418996798866, 18.362755587317, 18.306514375769, 18.250273164219, 18.194031952669, 18.137790741119,
        18.081549529568, 18.025308318017, 17.969067106465, 17.912825894913, 17.856584683360, 17.800343471807,
        17.744102260253, 17.687861048699, 17.631619837144, 17.575378625589, 17.519137414033, 17.462896202477,
        17.406654990920, 17.350413779363, 17.294172567806, 17.237931356248, 17.181690144690, 17.125448933131,
        17.069207721571, 17.012966510012, 16.956725298451, 16.900484086891, 16.844242875330, 16.788001663768,
        16.731760452206, 16.675519240644, 16.619278029081, 16.563036817517, 16.506795605954, 16.450554394389,
        16.394313182825, 16.338071971260, 16.281830759694, 16.225589548128, 16.169348336562, 16.113107124995,
        16.056865913428, 16.000624701860, 15.944383490292, 15.888142278724, 15.831901067155, 15.775659855586,
        15.719418644016, 15.663177432446, 15.606936220876, 15.550695009305, 15.494453797733, 15.438212586162,
        15.381971374590, 15.325730163017, 15.269488951444, 15.213247739871, 15.157006528297, 15.100765316723,
        15.044524105149, 14.988282893574, 14.932041681998, 14.875800470423, 14.819559258847, 14.763318047270,
        14.707076835694, 14.650835624116, 14.594594412539, 14.538353200961, 14.482111989383, 14.425870777804,
        14.369629566225, 14.313388354646, 14.257147143066, 14.200905931486, 14.144664719905, 14.088423508324,
        14.032182296743, 13.975941085162, 13.919699873580, 13.863458661998, 13.807217450415, 13.750976238832,
        13.694735027249, 13.638493815665, 13.582252604081, 13.526011392497, 13.469770180912, 13.413528969327,
        13.357287757741, 13.301046546156, 13.244805334570, 13.188564122983, 13.132322911396, 13.076081699809,
        13.019840488222, 12.963599276634, 12.907358065046, 12.851116853458, 12.794875641869, 12.738634430280,
        12.682393218691, 12.626152007101, 12.569910795511, 12.513669583921, 12.457428372330, 12.401187160739,
        12.344945949148, 12.288704737557, 12.232463525965, 12.176222314373, 12.119981102780, 12.063739891187,
        12.007498679594, 11.951257468001, 11.895016256407, 11.838775044813, 11.782533833219, 11.726292621625,
        11.670051410030, 11.613810198435, 11.557568986839, 11.501327775244, 11.445086563648, 11.388845352051,
        11.332604140455, 11.276362928858, 11.220121717261, 11.163880505664, 11.107639294066, 11.051398082468,
        10.995156870870, 10.938915659271, 10.882674447672, 10.826433236073, 10.770192024474, 10.713950812875,
        10.657709601275, 10.601468389675, 10.545227178074, 10.488985966474, 10.432744754873, 10.376503543272,
        10.320262331670, 10.264021120069, 10.207779908467, 10.151538696865, 10.095297485262, 10.039056273660,
        9.982815062057, 9.926573850454, 9.870332638851, 9.814091427247, 9.757850215643, 9.701609004039, 9.645367792435,
        9.589126580830, 9.532885369225, 9.476644157620, 9.420402946015, 9.364161734410, 9.307920522804, 9.251679311198,
        9.195438099592, 9.139196887985, 9.082955676379, 9.026714464772, 8.970473253165, 8.914232041558, 8.857990829950,
        8.801749618343, 8.745508406735, 8.689267195127, 8.633025983518, 8.576784771910, 8.520543560301, 8.464302348692,
        8.408061137083, 8.351819925473, 8.295578713864, 8.239337502254, 8.183096290644, 8.126855079034, 8.070613867424,
        8.014372655813, 7.958131444202, 7.901890232591, 7.845649020980, 7.789407809369, 7.733166597758, 7.676925386146,
        7.620684174534, 7.564442962922, 7.508201751310, 7.451960539697, 7.395719328085, 7.339478116472, 7.283236904859,
        7.226995693246, 7.170754481632, 7.114513270019, 7.058272058405, 7.002030846792, 6.945789635178, 6.889548423563,
        6.833307211949, 6.777066000335, 6.720824788720, 6.664583577105, 6.608342365490, 6.552101153875, 6.495859942260,
        6.439618730644, 6.383377519029, 6.327136307413, 6.270895095797, 6.214653884181, 6.158412672565, 6.102171460949,
        6.045930249332, 5.989689037715, 5.933447826099, 5.877206614482, 5.820965402865, 5.764724191248, 5.708482979630,
        5.652241768013, 5.596000556395, 5.539759344777, 5.483518133160, 5.427276921542, 5.371035709923, 5.314794498305,
        5.258553286687, 5.202312075068, 5.146070863450, 5.089829651831, 5.033588440212, 4.977347228593, 4.921106016974,
        4.864864805355, 4.808623593735, 4.752382382116, 4.696141170496, 4.639899958877, 4.583658747257, 4.527417535637,
        4.471176324017, 4.414935112397, 4.358693900777, 4.302452689156, 4.246211477536, 4.189970265915, 4.133729054295,
        4.077487842674, 4.021246631053, 3.965005419432, 3.908764207811, 3.852522996190, 3.796281784569, 3.740040572948,
        3.683799361327, 3.627558149705, 3.571316938084, 3.515075726462, 3.458834514840, 3.402593303218, 3.346352091597,
        3.290110879975, 3.233869668353, 3.177628456730, 3.121387245108, 3.065146033486, 3.008904821864, 2.952663610241,
        2.896422398619, 2.840181186996, 2.783939975374, 2.727698763751, 2.671457552128, 2.615216340506, 2.558975128883,
        2.502733917260, 2.446492705637, 2.390251494014, 2.334010282391, 2.277769070768, 2.221527859144, 2.165286647521,
        2.109045435898, 2.052804224275, 1.996563012651, 1.940321801028, 1.884080589404, 1.827839377781, 1.771598166157,
        1.715356954533, 1.659115742910, 1.602874531286, 1.546633319662, 1.490392108039, 1.434150896415, 1.377909684791,
        1.321668473167, 1.265427261543, 1.209186049919, 1.152944838295, 1.096703626671, 1.040462415047, 0.984221203423,
        0.927979991799, 0.871738780175, 0.815497568551, 0.759256356927, 0.703015145303, 0.646773933679, 0.590532722054,
        0.534291510430, 0.478050298806, 0.421809087182, 0.365567875558, 0.309326663933, 0.253085452309, 0.196844240685,
        0.140603029061, 0.084361817436, 0.028120605812 ) )

}  // namespace gaussian
}  // namespace spacing
}  // namespace grid
}  // namespace atlas

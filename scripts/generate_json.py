import jsontree 


base_url = 'https://github.com/jkoan/gh_actions_mapserver/releases/download/'
release = '2019_09_12'
continents = ['africa', 'antarctica', 'asia', 'australia_oceania', 'central_america', 'antarctica', 'europe', 'north_america', 'south_america']

countries_europe_subregion = ['france', 'germany', 'great_britain', 'italy', 'netherlands', 'poland']
countries_europe = ['albania', 'andorra', 'austria', 'azores', 'belarus', 'belgium', 'bosnia_herzegovina', 'bulgaria', 'croatia', 'cyprus', 'czech_republic', 'denmark', 'estonia', 'faroe_islands', 'finland','france',  'georgia', 'germany', 'great_britan', 'greece', 'guernsey_jersey', 'hungary', 'iceland', 'ireland_and_northern_ireland', 'isle_of_man', 'italy', 'kosovo', 'latvia', 'liechtenstein', 'lithuania', 'luxembourg', 'macedonia', 'malta', 'moldova', 'monaco', 'montenegro', 'netherlands', 'norway', 'poland', 'portugal', 'romania', 'serbia', 'slovakia', 'slovenia', 'spain', 'sweden', 'switzerland', 'turkey', 'ukraine'] 
countries_africa = ['algeria', 'angola', 'benin', 'botswana', 'burkina_faso', 'burundi', 'cameroon', 'canary_islands', 'cape_verde', 'central_african_republic', 'chad', 'comores', 'congo_brazzaville', 'congo_democratic_republic', 'djibouti', 'egypt', 'equatorial_guinea', 'eritrea', 'ethiopia', 'gabon', 'ghana', 'guinea', 'guinea_bissau', 'ivory_coast', 'kenya', 'lesotho', 'liberia', 'libya', 'madagascar', 'malawi', 'mali', 'mauritania', 'mauritius', 'morocco', 'mozambique', 'namibia', 'niger', 'nigeria', 'rwanda', 'saint_helena_ascension_and_tristan_da_cunha', 'sao_tome_and_principe', 'senegal_and_gambia', 'seychelles', 'sierra_leone', 'somalia', 'south_africa', 'south_sudan', 'sudan', 'swaziland', 'tanzania', 'togo', 'tunisia', 'uganda', 'zambia', 'zimbabwe']
countries_asia = ['afghanistan', 'armenia', 'azerbaijan', 'bangladesh', 'bhutan', 'cambodia', 'china', 'gcc_states', 'india', 'indonesia', 'iran', 'iraq', 'israel_and_palestine', 'japan', 'jordan', 'kazakhstan', 'kyrgyzstan', 'laos', 'lebanon', 'malaysia_singapore_brunei', 'maldives', 'mongolia', 'myanmar', 'nepal', 'north_korea', 'pakistan', 'philippines', 'south_korea', 'sri_lanka', 'syria', 'taiwan', 'tajikistan', 'thailand', 'turkmenistan', 'uzbekistan', 'vietnam', 'yemen'] 
countries_australia_oceania = ['american_oceania', 'australia', 'cook_islands', 'fiji', 'ile_de_clipperton', 'kiribati', 'marshall_islands', 'micronesia', 'nauru', 'new_caledonia', 'new_zealand', 'niue', 'palau', 'papua_new_guinea', 'pitcairn_islands', 'polynesie_francaise', 'samoa', 'solomon_islands', 'tokelau', 'tonga', 'tuvalu', 'vanuatu', 'wallis_et_futuna']
countries_central_america = ['bahamas', 'belize', 'costa_rica', 'cuba', 'el_salvador', 'guatemala', 'haiti_and_domrep', 'honduras', 'jamaica', 'nicaragua']
countries_north_america_subregion = ['canada', 'us']
countries_north_america = ['canada', 'greenland','mexico', 'us']
countries_south_america_subregion = ['brazil']
countries_south_america = ['argentina','bolivia','brazil','chile','colombia','ecuador','paraguay','peru','suriname','uruguay','venezuela']
regions_france = ['alsace','aquitaine','auvergne','basse_normandie','bourgogne','bretagne','centre','champagne_ardenne','corse','franche_comte','guadeloupe','guyane','haute_normandie','ile_de_france','languedoc_roussillon','limousin','lorraine','martinique','mayotte','midi_pyrenees','nord_pas_de_calais','pays_de_la_loire','picardie','poitou_charentes','provence_alpes_cote_d_azur','reunion','rhone_alpes']
regions_germany = ['baden_wuerttemberg','bayern','berlin','brandenburg','bremen','hamburg','hessen','mecklenburg_vorpommern','niedersachsen','nordrhein_westfalen','rheinland_pfalz','saarland','sachsen_anhalt','sachsen','schleswig_holstein','thueringen']
regions_great_britain = ['england','scotland','wales']
regions_italy = ['centro','isole','nord_est','nord_ovest','sud']
regions_netherlands = ['drenthe','flevoland','friesland','gelderland','groningen','limburg','noord_brabant','noord_holland','overijssel','utrecht','zeeland','zuid_holland']
regions_poland = ['dolnoslaskie','kujawsko_pomorskie','lodzkie','lubelskie','lubuskie','malopolskie','mazowieckie','opolskie','podkarpackie','podlaskie','pomorskie','slaskie','swietokrzyskie','warminsko_mazurskie','wielkopolskie','zachodniopomorskie']
regions_canada = ['alberta','british_columbia','manitoba','new_brunswick','newfoundland_and_labrador','northwest_territories','nova_scotia','nunavut','ontario','prince_edward_island','quebec','saskatchewan','yukon']
regions_us = ['alabama','alaska','arizona','arkansas','california','colorado','connecticut','delaware','district_of_columbia','florida','georgia','hawaii','idaho','illinois','indiana','iowa','kansas','kentucky','louisiana','maine','maryland','massachusetts','michigan','minnesota','mississippi','missouri','montana','nebraska','nevada','new_hampshire','new_jersey','new_mexico','new_york','north_carolina','north_dakota','ohio','oklahoma','oregon','pennsylvania','puerto_rico','rhode_island','south_carolina','south_dakota','tennessee','texas','utah','vermont','virginia','washington','west_virginia','wisconsin','wyoming']
regions_brazil = ['centro_oeste','nordeste','norte','sudeste','sul']

data = jsontree.jsontree()

for continent in continents:
    if continent == 'europe':
        for country in countries_europe:
            if country in countries_europe_subregion:
                for region in eval("regions_" + country):
                    exec("data.planet." + continent + "." + country + "." + region + ".filesize = 0")
              
            else:
                exec("data.planet." + continent + "." + country + ".filesize = 0")

    elif continent == 'north_america':
        for country in countries_north_america:
            if country in countries_north_america_subregion:
                for region in eval("regions_" + country):
                    exec("data.planet." + continent + "." + country + "." + region + ".filesize = 0")

            else:
                exec("data.planet." + continent + "." + country + ".filesize = 0")
    elif continent == 'south_america':
        for country in countries_south_america:
            if country in countries_south_america_subregion:
                for region in eval("regions_" + country):
                    exec("data.planet." + continent + "." + country + "." + region + ".filesize = 0")
            else:
                exec("data.planet." + continent + "." + country + ".filesize = 0")
    elif continent == 'antarctica':
        data.planet.antarctica.filesize = 0
    else:
        for country in eval("countries_" + continent):
            exec("data.planet." + continent + "." + country + ".filesize = 0")

file = open('./menu.json', 'w')
file.write(jsontree.dumps(data))

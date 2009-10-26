include $(top_srcdir)/Makefile.inc
AM_CPPFLAGS = @NAVIT_CFLAGS@ -DMODULE=support_espeak
noinst_LTLIBRARIES = libsupport_espeak.la
libsupport_espeak_la_SOURCES = compiledict.c dictionary.c intonation.c \
	readclause.c setlengths.c numbers.c synth_mbrola.c \
	synthdata.c synthesize.c translate.c \
	tr_languages.c voices.c wavegen.c phonemelist.c klatt.c speak_init.c
libsupport_espeak_la_LDFLAGS = -module -avoid-version @MODULE_LDFLAGS@

#bin_PROGRAMS = speak

#speak_SOURCES = speak.c $(libsupport_espeak_la_SOURCES)

nobase_pkgdata_DATA  = espeak-data/af_dict espeak-data/ca_dict espeak-data/config espeak-data/cs_dict espeak-data/cy_dict
nobase_pkgdata_DATA += espeak-data/da_dict espeak-data/de_dict espeak-data/el_dict espeak-data/en_dict espeak-data/eo_dict
nobase_pkgdata_DATA += espeak-data/es_dict espeak-data/fi_dict espeak-data/fr_dict espeak-data/grc_dict espeak-data/hbs_dict
nobase_pkgdata_DATA += espeak-data/hi_dict espeak-data/hu_dict espeak-data/hy_dict espeak-data/id_dict espeak-data/is_dict
nobase_pkgdata_DATA += espeak-data/it_dict espeak-data/jbo_dict espeak-data/ku_dict espeak-data/la_dict espeak-data/lv_dict
nobase_pkgdata_DATA += espeak-data/mk_dict espeak-data/nl_dict espeak-data/no_dict
nobase_pkgdata_DATA += espeak-data/pap_dict espeak-data/phondata espeak-data/phondata-manifest espeak-data/phonindex
nobase_pkgdata_DATA += espeak-data/phontab espeak-data/pl_dict espeak-data/pt_dict espeak-data/ro_dict espeak-data/ru_dict
nobase_pkgdata_DATA += espeak-data/sk_dict espeak-data/sq_dict espeak-data/sv_dict espeak-data/sw_dict espeak-data/ta_dict
nobase_pkgdata_DATA += espeak-data/tr_dict espeak-data/vi_dict espeak-data/voices/!v/croak espeak-data/voices/!v/f1
nobase_pkgdata_DATA += espeak-data/voices/!v/f2 espeak-data/voices/!v/f3 espeak-data/voices/!v/f4 espeak-data/voices/!v/fast
nobase_pkgdata_DATA += espeak-data/voices/!v/m1 espeak-data/voices/!v/m2 espeak-data/voices/!v/m3 espeak-data/voices/!v/m4
nobase_pkgdata_DATA += espeak-data/voices/!v/m5 espeak-data/voices/!v/m6 espeak-data/voices/!v/m7 espeak-data/voices/!v/whisper
nobase_pkgdata_DATA += espeak-data/voices/af espeak-data/voices/bs espeak-data/voices/ca espeak-data/voices/cs
nobase_pkgdata_DATA += espeak-data/voices/cy espeak-data/voices/da espeak-data/voices/de espeak-data/voices/default
nobase_pkgdata_DATA += espeak-data/voices/el espeak-data/voices/en/en espeak-data/voices/en/en-n espeak-data/voices/en/en-rp
nobase_pkgdata_DATA += espeak-data/voices/en/en-sc espeak-data/voices/en/en-us espeak-data/voices/en/en-wi
nobase_pkgdata_DATA += espeak-data/voices/en/en-wm espeak-data/voices/eo espeak-data/voices/es espeak-data/voices/es-la
nobase_pkgdata_DATA += espeak-data/voices/fi espeak-data/voices/fr espeak-data/voices/fr-be espeak-data/voices/hi
nobase_pkgdata_DATA += espeak-data/voices/hr espeak-data/voices/hu espeak-data/voices/hy espeak-data/voices/hy-west
nobase_pkgdata_DATA += espeak-data/voices/id espeak-data/voices/is espeak-data/voices/it espeak-data/voices/ku
nobase_pkgdata_DATA += espeak-data/voices/la espeak-data/voices/lv 
nobase_pkgdata_DATA += espeak-data/voices/mk espeak-data/voices/nl espeak-data/voices/no espeak-data/voices/pl
nobase_pkgdata_DATA += espeak-data/voices/pt espeak-data/voices/pt-pt espeak-data/voices/ro espeak-data/voices/ru
nobase_pkgdata_DATA += espeak-data/voices/sk espeak-data/voices/sq espeak-data/voices/sr espeak-data/voices/sv
nobase_pkgdata_DATA += espeak-data/voices/sw espeak-data/voices/ta espeak-data/voices/test/grc espeak-data/voices/test/jbo
nobase_pkgdata_DATA += espeak-data/voices/test/pap espeak-data/voices/tr espeak-data/voices/vi espeak-data/voices/zh 
nobase_pkgdata_DATA += espeak-data/voices/zh-yue espeak-data/zh_dict espeak-data/zhy_dict

#nobase_pkgdata_DATA += espeak-data/mbrola_ph/af1_phtrans espeak-data/mbrola_ph/ca1_phtrans espeak-data/mbrola_ph/cr1_phtrans
#nobase_pkgdata_DATA += espeak-data/mbrola_ph/cs_phtrans espeak-data/mbrola_ph/de2_phtrans espeak-data/mbrola_ph/de4_phtrans
#nobase_pkgdata_DATA += espeak-data/mbrola_ph/de6_phtrans espeak-data/mbrola_ph/en1_phtrans espeak-data/mbrola_ph/es_phtrans
#nobase_pkgdata_DATA += espeak-data/mbrola_ph/fr1_phtrans espeak-data/mbrola_ph/gr2_phtrans espeak-data/mbrola_ph/grc-de6_phtrans
#nobase_pkgdata_DATA += espeak-data/mbrola_ph/hu1_phtrans espeak-data/mbrola_ph/id1_phtrans espeak-data/mbrola_ph/in1_phtrans
#nobase_pkgdata_DATA += espeak-data/mbrola_ph/it3_phtrans espeak-data/mbrola_ph/la1_phtrans espeak-data/mbrola_ph/nl_phtrans
#nobase_pkgdata_DATA += espeak-data/mbrola_ph/pl1_phtrans espeak-data/mbrola_ph/pt1_phtrans espeak-data/mbrola_ph/pt_phtrans
#nobase_pkgdata_DATA += espeak-data/mbrola_ph/ptbr4_phtrans espeak-data/mbrola_ph/ptbr_phtrans espeak-data/mbrola_ph/ro1_phtrans
#nobase_pkgdata_DATA += espeak-data/mbrola_ph/sv2_phtrans espeak-data/mbrola_ph/sv_phtrans espeak-data/mbrola_ph/us3_phtrans
#nobase_pkgdata_DATA += espeak-data/mbrola_ph/us_phtrans
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-af1 espeak-data/voices/mb/mb-af1-en
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-br1 espeak-data/voices/mb/mb-br3 espeak-data/voices/mb/mb-br4
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-cr1 espeak-data/voices/mb/mb-cz2 espeak-data/voices/mb/mb-de2
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-de4 espeak-data/voices/mb/mb-de4-en espeak-data/voices/mb/mb-de5
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-de5-en espeak-data/voices/mb/mb-de6 espeak-data/voices/mb/mb-de6-grc
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-de7 espeak-data/voices/mb/mb-en1 espeak-data/voices/mb/mb-es1
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-es2 espeak-data/voices/mb/mb-fr1 espeak-data/voices/mb/mb-fr1-en
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-fr4 espeak-data/voices/mb/mb-fr4-en espeak-data/voices/mb/mb-gr2
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-gr2-en espeak-data/voices/mb/mb-hu1 espeak-data/voices/mb/mb-hu1-en
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-id1 espeak-data/voices/mb/mb-it3 espeak-data/voices/mb/mb-it4
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-la1 espeak-data/voices/mb/mb-nl2 espeak-data/voices/mb/mb-nl2-en
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-pl1 espeak-data/voices/mb/mb-pl1-en espeak-data/voices/mb/mb-pt1
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-ro1 espeak-data/voices/mb/mb-ro1-en espeak-data/voices/mb/mb-sw1
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-sw1-en espeak-data/voices/mb/mb-sw2 espeak-data/voices/mb/mb-sw2-en
#nobase_pkgdata_DATA += espeak-data/voices/mb/mb-us1 espeak-data/voices/mb/mb-us2 espeak-data/voices/mb/mb-us3

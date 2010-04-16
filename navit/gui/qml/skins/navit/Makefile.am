include $(top_srcdir)/Makefile.inc

SKIN = navit

qmldir=$(skinsdir)/$(SKIN)

qml_DATA = ButtonIcon.qml ToggleSwitch.qml ToggleButton.qml ListSelector.qml Slider.qml CommonHighlight.qml Cellar.qml main.qml command.qml PageNavigate.qml PageAbout.qml PageBookmarksAdd.qml PageBookmarks.qml PageNavigation.qml PageSearch.qml PageSearchSelector.qml PageSettingsDisplay.qml PageSettingsLocale.qml PageSettings.qml PageSettingsRules.qml PageSettingsTools.qml PageSettingsVehicle.qml PagePoi.qml PagePointInfo.qml
qml_DATA += background.svg knob.svg

do_button_subst = sed -e 's,matchProperties,properties,g' \
		      -e 's,easing\:\ \"ease,easing\.type\:\ \",g'

do_connections_subst = sed -e 's,sender:,target:,g' \
			   -e 's,signal\:\ ,on,g' \
			   -e 's,()\;\ script\:,\:,g' \
			   -e 's,Connection\ {,Connections\ {,g'

ButtonIcon.qml: ButtonIcon.qml.in
if QT_DECLARATIVE_NEWAPI
	$(do_button_subst) < ButtonIcon.qml.in > ButtonIcon.qml
else 
	cp ButtonIcon.qml.in ButtonIcon.qml
endif

ToggleSwitch.qml: ToggleSwitch.qml.in
if QT_DECLARATIVE_NEWAPI
	$(do_button_subst) < ToggleSwitch.qml.in > ToggleSwitch.qml
else 
	cp ToggleSwitch.qml.in ToggleSwitch.qml
endif

ToggleButton.qml: ToggleButton.qml.in
if QT_DECLARATIVE_NEWAPI
	$(do_button_subst) < ToggleButton.qml.in > ToggleButton.qml
else 
	cp ToggleButton.qml.in ToggleButton.qml
endif

PageSearchSelector.qml: PageSearchSelector.qml.in
if QT_DECLARATIVE_NEWAPI
	$(do_connections_subst) <  PageSearchSelector.qml.in >  PageSearchSelector.qml
else 
	cp  PageSearchSelector.qml.in  PageSearchSelector.qml
endif

clean-local:
	-$(RM) ButtonIcon.qml ToggleSwitch.qml ToggleButton.qml PageSearchSelector.qml

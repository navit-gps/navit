Navit is highly modular and customizable. This page aims to point out the most common options which a first-time user may want to change - power users may want to consult [[Configuration/Full_list_of_options | the full list of options]].
It is also possible to edit the navit.xml file for your Android device under Windows and Linux (Debian/Ubuntu derivates) with a third party application called [[NavitConfigurator]].

Setting up Navit is done by editing a configuration file called "**navit.xml**".
Editing XML configurations files in a text editor is simple, they are just plain text XML files, that can be edited with any editor. Just remember to ''turn off 'save UTF8 byte mark' in Preferences'' or navit may complain very much on the first byte of the file.<br>
The XML configuration file is splitted into sections within a hierarchy:

.. code-block:: xml

<source lang="xml">
<config>
  <plugins></plugins>
  <navit>
    <osd></osd>
    <vehicle></vehicle>
    <vehicleprofile></vehicleprofile>
    <mapset></mapset>
    <layout></layout>
  </navit>
</config>
</source>

Navit comes **preshipped** with a default ``navit.xml`` together with ``navit_layout_*.xml`` files that are stored at various locations (depending on your system). For Linux-like OSes:
* in ``~/.navit/``: e.g: ``/home/myusername/.navit/navit.xml`` (This is probably to best place to customize your settings!)
* in ``/usr/share/navit`` or ``/etc/navit``

Navit will apply settings in the following order:
* in the current directory (used on Windows)
* location supplied as first argument on the command line, e.g.: ``navit /home/myusername/navittestconfig.xml`` (Used mainly for development)
* in the current directory as ``navit.xml.local`` (Used mainly for development)

{{note
|In any case, you have to **adapt settings** to your system!<br> This includes especially GPS, map provider and vehicle: [[Basic configuration]]
}}

=Configurable Sections=
<div style="margin-bottom:8px;">
{| cellpadding=3 cellspacing=1

<!-- GENERAL -->
| width="33%" valign="top" style="background:#FFFFFF; border:2px solid #478cff;" |
{| style="width:100%; vertical-align:top; background:#FFFFFF;"
! style="padding:2px" | <h2 style="margin:3px; background:#478cff; font-size:120%; font-weight:bold; border:1px solid #478cff; text-align:left; color:#fff; padding:0.2em 0.4em;">General</h2>
|-
| style="margin:2px; padding:10px;" |

Common options such as units, position, zoom and map orientation, ... be configured in this section.

[[Configuration/General_Options| General options.]]

|}

| style="border:1px solid transparent;" |

| width="33%" valign="top" style="background:#FFFFFF; border:2px solid #48be1f; margin-top:50px;" |
<!-- DISPLAY -->
{| style="width:100%; vertical-align:top; background:#FFFFFF;"
|-
! style="padding:2px" | <h2 style="margin:3px; background:#48be1f; font-size:120%; font-weight:bold; border:1px solid #48be1f; text-align:left; color:#fff; padding:0.2em 0.4em;">Display</h2>
|-
| style="margin:2px; padding:10px;" |

A large number of display properties can be configured, including desktop or touchscreen-optimised GUIs, on-screen display items and complete control over menu items.

[[Configuration/Display_Options| Display options.]]

|}

| style="border:1px solid transparent;" |


<!-- VEHICLE -->
| width="33%" valign="top" style="background:#FFFFFF; border:2px solid #e79e00;" |
{| style="width:100%; vertical-align:top; background:#FFFFFF;"
! style="padding:2px" | <h2 style="margin:3px; background:#e79e00; font-size:120%; font-weight:bold; border:0px solid #e79e00; text-align:left; color:#fff; padding:0.2em 0.4em;">Vehicle</h2>
|-
| style="margin:2px; padding:10px;" |

A number of vehicles can be defined within Navit, depending upon the device and/or operating system in use. Vehicle profiles for routing (eg: car, pedestrian, bicycle...) are also completely configurable.

[[Configuration/Vehicle_Options| Vehicle options.]]

|}
|}
</div>

<div style="margin-bottom:8px;">
{| cellpadding=3 cellspacing=1

<!-- MAPS -->
| width="33%" valign="top" style="background:#FFFFFF; border:2px solid #ec7312;" |
{| style="width:100%; vertical-align:top; background:#FFFFFF;"
! style="padding:2px" | <h2 style="margin:3px; background:#ec7312; font-size:120%; font-weight:bold; border:1px solid #ec7312; text-align:left; color:#fff; padding:0.2em 0.4em;">Maps</h2>
|-
| style="margin:2px; padding:10px;" |

You can use maps from a variety of sources, any number of maps can be configured and enabled at any one time.

[[Configuration/Maps_Options| Maps options.]]

|}

| style="border:1px solid transparent;" |

<!-- LAYOUT -->
| width="33%" valign="top" style="background:#FFFFFF; border:2px solid #b30800;" |
{| style="width:100%; vertical-align:top; background:#FFFFFF;"
! style="padding:2px" | <h2 style="margin:3px; background:#b30800; font-size:120%; font-weight:bold; border:1px solid #b30800; text-align:left; color:#fff; padding:0.2em 0.4em;">Layout</h2>
|-
| style="margin:2px; padding:10px;" |

Maps are displayed according to the rules defined in the layout. All aspects of the layout are configurable, from POI icons to colours for a particular type of highway.

For all versions shipped after nov 2018, layout XML configuration is stored in dedicated XML files called with the prefix **navit_layout_** (one file per layout definition).

[[Configuration/Layout_Options| Layout options.]]

|}

| style="border:1px solid transparent;" |

<!-- ADVANCED -->
| width="33%" valign="top" style="background:#FFFFFF; border:2px solid #992667;" |
{| style="width:100%; vertical-align:top; background:#FFFFFF;"
! style="padding:2px" | <h2 style="margin:3px; background:#992667; font-size:120%; font-weight:bold; border:1px solid #992667; text-align:left; color:#fff; padding:0.2em 0.4em;">Advanced</h2>
|-
| style="margin:2px; padding:10px;" |

There are many more options, including debugging, specific plugins, speech announcements,  trip logging, ...

[[Configuration/Advanced_Options| Advanced options.]]

|}


|}
</div>

<!-- Following line disables table of contents -->
__NOTOC__

[[Category:Customizing]]
[[Category:Configuration]]

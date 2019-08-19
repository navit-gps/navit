#!/usr/bin/env python

# export to SVG or PNG (incl. retina output), re-colour, add padding,
# add halo, generate sprites

from __future__ import print_function
import argparse
import colorsys
import copy
import glob
import lxml.etree
import lxml.objectify
import math
import os
import re
import shutil
import subprocess
import sys
import yaml


def main():
    parser = argparse.ArgumentParser(description='Exports Osmic (OSM Icons).')
    parser.add_argument('configfile',
                        metavar='config-file',
                        help='the configuration file for the export')
    parser.add_argument('--basedir', dest='basedir',
                        metavar='base-directory',
                        help='specify the working directory of the script',
                        required=False,
                        default=None)
    parser.add_argument('--input', dest='input_basedir',
                        metavar='input base-directory',
                        help='specify the base directory for the input',
                        required=False,
                        default=None)
    parser.add_argument('--output', dest='output_basedir',
                        metavar='output base-directory',
                        help='specify the base directory for the output',
                        required=False,
                        default=None)
    args = parser.parse_args()

    try:
        configfile = open(args.configfile)
    except IOError:
        sys.exit('Could not open configuration file, please check if it exists. Exiting.')

    config = yaml.safe_load(configfile)
    configfile.close()

    defaultValues(config)

    # command line specified basedir values overwrites config file values
    if args.basedir:
        config['basedir'] = args.basedir
    if args.input_basedir:
        config['input_basedir'] = args.input_basedir
    if args.output_basedir:
        config['output_basedir'] = args.output_basedir

    # set basedir (evaluation from cwd, if not specified always cwd)
    if 'basedir' in config:
        os.chdir(os.path.dirname(config['basedir']))

    # empty output directory if it exists and config param is set
    if config['empty_output'] == True and os.path.exists(config['output_basedir']):
        for the_file in os.listdir(config['output_basedir']):
            file_path = os.path.join(config['output_basedir'], the_file)
            try:
                if os.path.isfile(file_path):
                    os.unlink(file_path)
                elif os.path.isdir(file_path):
                    shutil.rmtree(file_path)
            except:
                pass

    if not isinstance(config['retina'], bool):
        config['retina'] = False
        print('The retina flag must be boolean. Defaulting to false.')

    if not isinstance(config['dpi'], int):
        config['dpi'] = 90
        print('The dpi parameter must be a number. Defaulting to 90 dpi.')

    # filter by size
    size_filter = 0

    if 'size_filter' in config:
        try:
            size_filter = int(config['size_filter'])

            if size_filter < 0:
                print('A negative number of pixels for the size filter is not allowed.')
        except ValueError:
            print('The size filter is not a number.')

    num_icons = 0

    # loop through all specified directories
    for directory in config['input']:
        if 'name' not in directory:
            print('This directory has no name (name: directory). Skipping.')
            continue
        if 'include' in directory and 'exclude' in directory:
            print('Include and exclude cannot be specified together for one directory. Skipping.')
            continue

        filter_spec = None
        if 'include' in directory:
            filter_spec = directory['include']
        elif 'exclude' in directory:
            filter_spec = directory['exclude']

        icon_filter = None
        if filter_spec:
            icon_filter = dict()
            for element in filter_spec:
                name_match = re.search('^([a-z-]+)([0-9]+)?$', element)
                icon_id = None
                size = None
                if name_match is not None:
                    icon_id = name_match.group(1).rstrip('-')
                    if name_match.group(2) is not None:
                        size = int(name_match.group(2))
                if icon_id is not None:
                    if icon_id not in icon_filter:
                        icon_filter[icon_id] = []
                    if size is not None:
                        icon_filter[icon_id].append(size)

        dir_path = os.path.join(config['input_basedir'], directory['name'])

        # loop through all SVG files in this directory
        for icon_path in glob.glob(os.path.join(dir_path, '*.svg')):
            name_match = re.search('^([a-z-]+)\-([0-9]+)$', os.path.splitext(os.path.basename(icon_path))[0])
            icon_id = None
            size = None
            if name_match is not None:
                icon_id = name_match.group(1)
                size = int(name_match.group(2))

            # include mode - determine if current file should be included in set
            if 'include' in directory:
                if icon_id not in icon_filter:
                    continue
                elif icon_filter[icon_id]:
                    if size not in icon_filter[icon_id]:
                        continue

            # exclude mode - determine if current file should be excluded from set
            if 'exclude' in directory and icon_id in icon_filter:
                if icon_filter[icon_id]:
                    if size in icon_filter[icon_id]:
                        continue
                else:
                    continue

            # skip files that do not match size filter
            if (size_filter > 0 and size != size_filter):
                continue

            # read in file contents
            try:
                iconfile = open(icon_path)
            except IOError:
                continue
            icon = iconfile.read()
            iconfile.close()

            # add remove canvas option for font output
            if config['format'] == 'font':
                config['global_style']['canvas'] = False

            # override global config with icon specific options
            mod_config = copy.deepcopy(config['global_style'])
            if icon_id in config:
                for option in config[icon_id]:
                    # when generating sprites only the fill colour can be overridden
                    if not config['format'] == 'sprite' or option == 'fill':
                        mod_config[option] = config[icon_id][option]

            try:
                # do modifications
                (size, icon) = modifySVG(mod_config, icon_id, size, icon)

                # create subdirs
                if not config['format'] == 'font':
                    if config['subdirs'] == True:
                        icon_out_path = os.path.join(config['output_basedir'], directory['name'], icon_id + '-' + str(size) + '.svg')
                    else:
                        icon_out_path = os.path.join(config['output_basedir'], icon_id + '-' + str(size) + '.svg')
                else:
                    # remove subdirs and size info for font output
                    icon_out_path = os.path.join(config['output_basedir'], icon_id + '.svg')

                if not os.path.exists(os.path.dirname(icon_out_path)):
                    os.makedirs(os.path.dirname(icon_out_path))

                # save modified file
                try:
                    iconfile = open(icon_out_path, 'w')
                    iconfile.write(icon)
                    iconfile.close()
                except IOError:
                    print('Could not save the modified file ' + icon_out_path + '.')
                    continue

                num_icons += 1

                if config['format'] not in ['svg', 'png', 'sprite', 'font']:
                    print('Format must be either svg, png, sprite or font. Defaulting to svg.')

                # if PNG export generate PNG file and delete modified SVG
                if config['format'] == 'png':
                    if config['subdirs'] == True:
                        destination = os.path.join(config['output_basedir'], directory['name'], icon_id + '-' + str(size) + '.png')
                    else:
                        destination = os.path.join(config['output_basedir'], icon_id + '-' + str(size) + '.png')

                    exportPNG(icon_out_path, destination, config['dpi'], config['retina'])
                    os.remove(icon_out_path)
            except Exception, e:
                print(e)
                continue


    # export font
    if config['format'] == 'font':
        exportFont(config, size_filter)

    # generate sprite
    if config['format'] == 'sprite':
        exportSprite(config, size_filter, num_icons)

    return


# set config default values
def defaultValues(config):
    # config default values
    if 'input' not in config:
        config['input'] = ''

    if 'input_basedir' not in config:
        config['input_basedir'] = os.getcwd()

    if 'output_basedir' not in config:
        config['output_basedir'] = os.path.join(os.getcwd(), 'export')

    if 'empty_output' not in config:
        config['empty_output'] = False

    if 'format' not in config:
        config['format'] = 'png'

    if 'retina' not in config:
        config['retina'] = False

    if 'subdirs' not in config:
        config['subdirs'] = True

    if 'dpi' not in config:
        config['dpi'] = 90

    if 'global_style' not in config:
        config['global_style'] = {}

    if config['format'] == 'font':
        if 'font' not in config:
            config['font'] = {}

        if 'size_filter' not in config:
            config['size_filter'] = 14

        if 'output_basedir' not in config['font']:
            config['font']['output_basedir'] = os.path.join(os.getcwd(), 'font')

    return


def parseIconSizeParams(config):
    # padding of icon
    padding = 0
    if 'padding' in config:
        try:
            padding = int(config['padding'])
            if padding < 0:
                padding = 0
        except ValueError:
            pass

    # halo of icon
    halo_width = 0
    if 'halo' in config and 'width' in config['halo']:
        try:
            halo_width = float(config['halo']['width'])

            if halo_width < 0:
                halo_width = 0
        except ValueError:
            pass

    # shield width
    shield = 0
    if 'shield' in config:
        if 'padding' in config['shield']:
            try:
                shield_padding = int(config['shield']['padding'])

                if shield_padding > 0:
                    shield += int(config['shield']['padding']) * 2
            except ValueError:
                pass

        stroke_width = 0
        if 'stroke_width' in config['shield']:
            try:
                stroke_width = float(config['shield']['stroke_width'])
                if stroke_width > 0:
                    shield += stroke_width * 2
                elif stroke_width < 0:
                    shield += 2
            except ValueError:
                pass

    return (padding, halo_width, shield)


# export Sprite
def exportSprite(config, size_filter, num_icons):
    if 'sprite' in config:
        sprite_cols = 12
        if 'cols' in config['sprite']:
            try:
                sprite_cols = int(config['sprite']['cols'])

                if sprite_cols <= 0:
                    print('A negative number of sprite columns or zero columns are not allowed. Defaulting to 12 columns.')
            except ValueError:
                print('Sprite columns is not a number. Defaulting to 12 columns.')

        outer_padding = 4
        if 'outer_padding' in config['sprite']:
            try:
                outer_padding = int(config['sprite']['outer_padding'])

                if outer_padding < 0:
                    print('A negative number of sprite outer padding is not allowed. Defaulting to a padding of 4.')
            except ValueError:
                print('Sprite outer padding is not a number. Defaulting to a padding of 4.')

        icon_padding = 4
        if 'icon_padding' in config['sprite']:
            try:
                icon_padding = int(config['sprite']['icon_padding'])

                if icon_padding < 0:
                    print('A negative number of sprite icon padding is not allowed. Defaulting to a padding of 4.')
            except ValueError:
                print('Sprite outer padding is not a number. Defaulting to a padding of 4.')

        sprite_background = None
        if 'background' in config['sprite']:
            sprite_background = parseColor(config['sprite']['background'])
            if sprite_background == None:
                print('The specified background colour is invalid. Format it as HEX/RGB/HSL/HUSL (e.g. #1a1a1a). Defaulting to none (transparent).')

        sprite_file_name = 'sprite'
        if 'filename' in config['sprite']:
            sprite_file_name = config['sprite']['filename']
        sprite_out_path = os.path.join(config['output_basedir'], sprite_file_name)

        # have to define sane defaults for size parameter, otherwise the last read icon determines the size (that would be random)
        (padding, halo_width, shield_width) = parseIconSizeParams(config['global_style'])
        if size_filter > 0:
            size = size_filter + (padding + halo_width) * 2 + shield_width
        else:
            size = 14 + (padding + halo_width) * 2 + shield_width

        sprite_width = outer_padding * 2 + sprite_cols * (icon_padding * 2 + size)
        sprite_height = outer_padding * 2 + (icon_padding * 2 + size) * math.ceil(float(num_icons) / sprite_cols)

        sprite = lxml.etree.Element('svg')
        sprite.set('width', str(sprite_width))
        sprite.set('height', str(sprite_height))

        if sprite_background is not None:
            bg_rect = lxml.etree.Element('rect')
            bg_rect.set('width', str(sprite_width))
            bg_rect.set('height', str(sprite_height))
            bg_rect.set('style', 'fill:'+sprite_background+';')
            sprite.append(bg_rect)

        # loop through all specified directories (for sprite again)

        col = 1
        row = 1
        x = outer_padding + icon_padding
        y = outer_padding + icon_padding

        for directory in config['input']:
            if 'name' not in directory:
                continue
            if 'include' in directory and 'exclude' in directory:
                continue

            dir_path = os.path.join(config['output_basedir'], directory['name'])

            # loop through all SVG files in this directory (in alphabetical order)
            for icon_path in sorted(glob.glob(os.path.join(dir_path, '*.svg'))):
                name_match = re.search('^([a-z-]+)\-([0-9]+)$', os.path.splitext(os.path.basename(icon_path))[0])
                icon_id = None
                size = None
                if name_match is not None:
                    icon_id = name_match.group(1)
                    size = int(name_match.group(2))

                # read in file contents
                try:
                    iconfile = open(icon_path)
                except IOError:
                    continue
                icon_str = iconfile.read()
                iconfile.close()

                icon_xml = lxml.etree.fromstring(icon_str)
                icon_sprite = lxml.etree.Element('g')
                icon_sprite.set('id', icon_id)
                icon_sprite.set('transform', 'translate('+str(x)+','+str(y)+')')

                for child in list(icon_xml):
                    if child.attrib['id'] != 'metadata8' and child.attrib['id'] != 'defs6':
                        icon_sprite.append(child)

                sprite.append(icon_sprite)

                col += 1
                x += size + icon_padding * 2
                if col > sprite_cols:
                    row += 1
                    col = 1
                    x = outer_padding + icon_padding
                    y += size + icon_padding * 2

                # after adding to sprite delete SVG
                os.remove(icon_path)

            # after finishing directory remove it
            if os.path.isdir(dir_path):
                os.rmdir(dir_path)

        # save sprite SVG to file
        try:
            spritefile = open(sprite_out_path+'.svg', 'w')
            spritefile.write(lxml.etree.tostring(sprite, pretty_print=True))
            spritefile.close()
        except IOError:
            print('Could not save the sprite SVG file ' + sprite_out_path + '.svg' + '.')

        # export sprite as PNG
        exportPNG(sprite_out_path+'.svg', sprite_out_path+'.png', config['dpi'], config['retina'])

    return


# export PNG
def exportPNG(source, destination, dpi, retina):
    for i in range(0, 2):
        # TODO Windows?
        try:
            # rsvg is preferred because faster, but fallback to Inkscape when rsvg is not installed
            subprocess.call(['rsvg', '-a', '--zoom='+str(round(float(dpi) / 90, 2)), '--format=png', source, destination])
        except OSError:
            try:
                subprocess.call(['inkscape', '--export-dpi='+str(dpi), '--export-png='+destination, source])
            except OSError:
                # if neither is installed print a message and exit
                sys.exit('Export to PNG requires either rsvg or Inkscape. Please install one of those. rsvg seems to be faster (if you just want to export). Exiting.')

        if not retina:
            break
        else:
            dpi *= 2
            # append @2x to file name
            split_name = os.path.splitext(destination)
            destination = split_name[0]+'@2x'+split_name[1]

    return


# export icon font
def exportFont(config, size_filter):
    if 'font' in config:
        source = config['output_basedir']
        destination = config['font']['output_basedir']

        # have to define sane defaults for size parameter, otherwise the last read icon determines the size (that would be random)
        (padding, halo_width, shield_width) = parseIconSizeParams(config['global_style'])
        if size_filter > 0:
            size = size_filter + (padding + halo_width) * 2 + shield_width
        else:
            size = 14 + (padding + halo_width) * 2 + shield_width

        copyright_message = 'Osmic icon font (https://github.com/nebulon42/osmic), License: SIL OFL'
        # TODO Windows?
        try:
            subprocess.call(['fontcustom', 'compile', source, '--force', '--output=' + destination, '--font-name=osmic', '--no-hash', '--font-design-size=' + str(size), '--css-selector=.oc-{{glyph}}', '--copyright=' + copyright_message])

            # strip out metadata of SVG font
            svg_font = os.path.join(destination, 'osmic.svg')
            try:
                fontfile = open(svg_font)
                font = fontfile.read()
                fontfile.close()

                xml = lxml.etree.fromstring(font)
                xpEval = lxml.etree.XPathEvaluator(xml)
                xpEval.register_namespace('def', 'http://www.w3.org/2000/svg')
                metadata = xpEval('//def:metadata')[0]
                metadata.text = copyright_message
                lxml.etree.ElementTree(xml).write(svg_font, pretty_print=True)
            except IOError:
                # if we cannot read the file we just let it be
                pass
        except OSError:
            # fontcustom is not installed
            sys.exit('Export as icon font requires Font Custom. See http://fontcustom.com. Exiting.')
    return


# parse rgb, hex, hsl and husl (perceptual) color definitions, return hex color or None on error
# husl needs husl library, load on demand
def parseColor(color):
    parsed_color = None
    if color.startswith('#'):
        if re.match('^#[0-9a-f]{6}$', color) != None:
            parsed_color = color
    elif color.startswith('rgb'):
        rgb_match = re.search('^rgb\(([0-9]{1,3}),\s*([0-9]{1,3}),\s*([0-9]{1,3})\)$', color)
        if rgb_match is not None:
            try:
                r = min(int(rgb_match.group(1)), 255)
                g = min(int(rgb_match.group(2)), 255)
                b = min(int(rgb_match.group(3)), 255)
                parsed_color = '#'+("%x" % r)+("%x" % g)+("%x" % b)
            except ValueError:
                pass
    elif color.startswith('hsl'):
        hsl_match = re.search('^hsl\(([0-9]{1,3}(\.[0-9]*)?),\s*([0-9]{1,3}(\.[0-9]*)?)\%,\s*([0-9]{1,3}(\.[0-9]*)?)\%\)$', color)
        if hsl_match is not None:
            try:
                h = min(float(hsl_match.group(1)) / 365, 1)
                s = min(float(hsl_match.group(3)) / 100, 1)
                l = min(float(hsl_match.group(5)) / 100, 1)
                (r, g, b) = colorsys.hls_to_rgb(h, l, s)
                parsed_color = '#'+("%x" % (r * 255))+("%x" % (g * 255))+("%x" % (b * 255))
            except ValueError:
                pass
    elif color.startswith('husl'):
        husl_match = re.search('^husl\(([0-9]{1,3}(\.[0-9]*)?),\s*([0-9]{1,3}(\.[0-9]*)?)\%,\s*([0-9]{1,3}(\.[0-9]*)?)\%\)$', color)
        if husl_match is not None:
            try:
                import husl
                h = min(float(husl_match.group(1)), 360)
                s = min(float(husl_match.group(3)), 100)
                l = min(float(husl_match.group(5)), 100)
                rgb = husl.husl_to_rgb(h, s, l)
                parsed_color = husl.rgb_to_hex(rgb)
            except ValueError:
                pass
            except ImportError:
                print('HUSL colour definitions need the husl library. Please install it.')
                pass

    return parsed_color


# modifications to the SVG
def modifySVG(config, icon_id, size, icon):
    xml = lxml.etree.fromstring(icon)
    # cleanup namespaces as Inkscape adds a (unnecessary?) xmlns:svg namespace,
    # which leads to unwanted svg:path elements for halos
    lxml.objectify.deannotate(xml, cleanup_namespaces=True)
    xpEval = lxml.etree.XPathEvaluator(xml)
    xpEval.register_namespace('def', 'http://www.w3.org/2000/svg')

    # sanity checks
    paths = xpEval("//def:path")
    if len(paths) > 1:
        raise Exception("Icon '"+icon_id+"' contains more than one path. Skipping.")
    if paths[0].get("id") == None:
        raise Exception("Icon '"+icon_id+"' contains no path with id attribute. Skipping.")
    if paths[0].get("id") != icon_id:
        raise Exception("Icon file name ("+icon_id+") and path id attribute ("+paths[0].get("id")+") do not match. Skipping.")

    # set padding of icon
    padding = 0
    if 'padding' in config:
        try:
            padding = int(config['padding'])

            if padding < 0:
                padding = 0
                print('Negative padding is not allowed. Defaulting to 0.')
        except ValueError:
            print('Padding is not a number.')



    # add shield
    shield_size = size
    if 'shield' in config:
        if 'padding' in config['shield']:
            try:
                shield_padding = int(config['shield']['padding'])

                if shield_padding > 0:
                    shield_size += int(config['shield']['padding']) * 2
                else:
                    print('Negative shield padding is not allowed. Defaulting to 0.')
            except ValueError:
                print('Shield padding is not a number. Defaulting to 0.')

        shield_rounded = 0
        if 'rounded' in config['shield']:
            try:
                shield_rounded = int(config['shield']['rounded'])

                if shield_rounded <= 0:
                    shield_rounded = 0
                    print('A negative shield corner radius is not allowed. Defaulting to unrounded corners.')
            except ValueError:
                print('Shield corner radius is not a number. Defaulting to unrounded corners.')

        shield_fill = '#000000'
        if 'fill' in config['shield']:
            shield_fill = parseColor(config['shield']['fill'])
            if shield_fill == None:
                shield_fill = '#000000'
                print('The specified shield fill is invalid. Format it as HEX/RGB/HSL/HUSL (e.g. #1a1a1a). Defaulting to #000000 (black).')
        else:
            print('Shield fill not specified. Defaulting to #000000 (black).')

        shield_opacity = None
        if 'opacity' in config['shield']:
            try:
                shield_opacity = float(config['shield']['opacity'])

                if shield_opacity < 0 or shield_opacity > 1:
                    shield_opacity = 1.0
                    print('Fill opacity must lie between 0 and 1 (e.g. 0.5). Defaulting to 1.0.')
            except ValueError:
                print('The specified fill opacity is not a number.')

        stroke = 'stroke:none;'
        stroke_fill = None
        if 'stroke_fill' in config['shield']:
            stroke_fill = parseColor(config['shield']['stroke_fill'])
            if stroke_fill == None:
                print('The specified shield stroke fill is invalid. Format it as HEX/RGB/HSL/HUSL (e.g. #1a1a1a).')

        stroke_width = None
        if 'stroke_width' in config['shield']:
            try:
                stroke_width = float(config['shield']['stroke_width'])

                if stroke_width < 0:
                    stroke_width = 1
                    print('Negative shield stroke widths are not allowed. Defaulting to width=1.')
            except ValueError:
                print('The specified shield stroke width is not a number.')

        stroke_opacity = None
        if 'stroke_opacity' in config['shield']:
            try:
                stroke_opacity = float(config['shield']['stroke_opacity'])

                if stroke_opacity < 0 or stroke_opacity > 1:
                    stroke_opacity = 1.0
                    print('Stroke opacity must lie between 0 and 1 (e.g. 0.5). Opacities of 0 do not make sense. Defaulting to 1.0.')
            except ValueError:
                print('The specified stroke opacity is not a number.')

        if stroke_fill is not None and stroke_width is not None:
            # do not specify stroke if stroke_width = 0 was specified
            if stroke_width > 0:
                stroke = 'stroke:'+stroke_fill+';stroke-width:'+str(stroke_width)+';stroke-opacity:'+str(stroke_opacity)+';'
        else:
            # do not print warning if stroke width = 0 or none was specified
            if stroke_width > 0:
                print('Shield: Defined either stroke_fill without stroke_width or vice versa. Both are required for strokes to appear.')

        shield = lxml.etree.Element('rect')
        shield.set('x', str(padding))
        shield.set('y', str(padding))
        shield.set('width', str(shield_size))
        shield.set('height', str(shield_size))
        if shield_rounded > 0:
            shield.set('rx', str(shield_rounded))
            shield.set('ry', str(shield_rounded))
        shield.set('id', 'shield')
        shield.set('style', 'fill:'+shield_fill+';'+'fill-opacity:'+str(shield_opacity)+';'+stroke)

        canvas = xpEval("//def:rect[@id='canvas']")[0]
        canvas.addnext(shield)


    # add icon halo
    halo_width = 0
    if 'halo' in config:
        halo_fill = '#fffff'
        if 'fill' in config['halo']:
            halo_fill = parseColor(config['halo']['fill'])
            if halo_fill == None:
                halo_fill = '#ffffff'
                print('The specified halo fill is invalid. Format it as HEX/RGB/HSL/HUSL (e.g. #1a1a1a). Defaulting to #ffffff (white).')
        else:
            print('Halo fill not specified. Defaulting to #ffffff (white).')

        if 'width' in config['halo']:
            try:
                halo_width = float(config['halo']['width'])

                if halo_width < 0:
                    halo_width = 0
                    print('Halo widths < 0 do not make sense. Omitting halo.')
            except ValueError:
                print('The specified halo width is not a number.')

        halo_opacity = None
        if 'opacity' in config['halo']:
            try:
                halo_opacity = float(config['halo']['opacity'])

                if halo_opacity <= 0 or halo_opacity > 1:
                    halo_opacity = 0.3
                    print('Halo opacity must lie between 0 and 1 (e.g. 0.5). Opacities of 0 do not make sense. Defaulting to 0.3.')
            except ValueError:
                print('The specified halo opacity is not a number.')

        if not halo_width == 0:
            icon_element = xpEval("//def:path[@id='"+icon_id+"']")[0]
            halo = copy.deepcopy(icon_element)
            halo.set('id', 'halo')
            halo.set('style', 'fill:'+halo_fill+';stroke:'+halo_fill+';stroke-width:'+str(halo_width * 2)+';opacity:'+str(halo_opacity)+';')
            halo.set('transform', 'translate('+str(padding + halo_width)+','+str(padding + halo_width)+')')
            icon_element.addprevious(halo)


    # change fill colour of icon
    if 'fill' in config:
        fill_color = parseColor(config['fill'])
        if fill_color != None:
            path = xpEval("//def:path[@id='"+icon_id+"']")[0]
            path.attrib['style'] = re.sub('fill:#[0-9a-f]{6};?', 'fill:'+fill_color+';', path.attrib['style'])
        else:
            print('The specified fill is invalid. Format it as HEX/RGB/HSL/HUSL (e.g. #1a1a1a).')

    # change the icon opacity
    opacity = None
    if 'opacity' in config:
        try:
            opacity = float(config['opacity'])
            if opacity < 0 or opacity > 1:
                opacity = 1.0
                print('Icon opacity must lie between 0 and 1 (e.g. 0.5). Defaulting to 1.0.')
            else:
                path.attrib['style'] = re.sub('fill-opacity:[.0-9]+;?', 'fill-opacity:'+str(opacity)+';', path.attrib['style'])
        except ValueError:
            print('The specified icon opacity is not a number.')


    # adjust document and canvas size, icon position
    shieldIncrease = shield_size - size
    size += int(max((shield_size - size), halo_width * 2) + padding * 2)
    xml.attrib['viewBox'] = '0 0 ' + str(size) + ' ' + str(size)
    canvas = xpEval("//def:rect[@id='canvas']")[0]
    canvas.attrib['width'] = str(size)
    canvas.attrib['height'] = str(size)
    icon_xml = xpEval("//def:path[@id='"+icon_id+"']")[0]
    icon_xml.set('transform', 'translate('+str(max(shieldIncrease / 2, halo_width) + padding)+','+str(max(shieldIncrease / 2, halo_width) + padding)+')')

    # strip 'stroke:none' from style attributes
    icon_xml.attrib['style'] = re.sub(';stroke:none', '', icon_xml.attrib['style'])

    # remove canvas for font output
    if 'canvas' in config and config['canvas'] == False:
        canvas.getparent().remove(canvas)

    icon = lxml.etree.tostring(xml, encoding='utf-8', xml_declaration=True, pretty_print=True)
    return (size, icon)

if __name__ == "__main__":
    main()

#!/usr/local/bin/php &#8211;q
<?php
function getmercator($sx,$sy,$ex,$ey) {
	
	$sx = $sx*6371000.0*M_PI/180;
	$sy = log(tan(M_PI_4+$sy*M_PI/360))*6371000.0;
	
	$ex = $ex*6371000.0*M_PI/180;
	$ey = log(tan(M_PI_4+$ey*M_PI/360))*6371000.0;
	
	return array(
		'l' => array(
				'x' => $sx,
				'y' => $sy
			),
		'h' => array(
				'x' => $ex,
				'y' => $ey
			)
	);
	
}
function contains_bbox($c, &$r) {
	if ($c['l']['x'] > $r['h']['x'])
		return false;
	elseif ($c['h']['x'] < $r['l']['x'])
		return false;
	elseif ($c['l']['y'] > $r['h']['y'])
		return false;
	elseif ($c['h']['y'] < $r['l']['y'])
		return false;
	else 
		return true;
}

$fetch_bbox = getmercator(11.3, 47.9, 11.4, 48.0);

$files = array();
$files['input']  = '/home/burner/carputer/navit/src/maps/osm_bbox_11.3,47.9,11.7,48.2.bin';
$files['output'] = 'myarea.bin';

$formats = array();
$formats['ziphpack'] = "lssssslLLSS";
$formats['zipheader']  = "l" . "ziplocsig";	# Signature (is always the same)
$formats['zipheader'] .= "/s" . "zipver";	# zip version needed
$formats['zipheader'] .= "/s" . "zipgenfld";# type of os that generated the file
$formats['zipheader'] .= "/s" . "zipmthd";	# 
$formats['zipheader'] .= "/s" . "ziptime";	# time
$formats['zipheader'] .= "/s" . "zipdate";	# date
$formats['zipheader'] .= "/l" . "zipcrc";	# crc checksum
$formats['zipheader'] .= "/L" . "zipsize";	# data size
$formats['zipheader'] .= "/L" . "zipuncmp";	# uncompressed size
$formats['zipheader'] .= "/S" . "zipfnln";	# length of filename
$formats['zipheader'] .= "/S" . "zipxtraln";# length of extra data (always 0)

$formats['zipcd'] = "".
	"i" . "zipcensig/".
	"c" . "zipcver/".
	"c" . "zipcos/".
	"c" . "zipcvxt/".
	"c" . "zipcexos/".
	"s" . "zipcflg/".
	"s" . "zipcmthd/".
	"s" . "ziptim/".
	"s" . "zipdat/".
	"i" . "zipccrc/".
	"I" . "zipcsiz/".
	"I" . "zipcunc/".
	"S" . "zipcfnl/".
	"S" . "zipcxtl/".
	"S" . "zipccml/".
	"S" . "zipdsk/".
	"S" . "zipint/".
	"I" . "zipext/".
	"I" . "zipofst/".
$formats['zipcdpack'] = "iccccssssiIISSSSSII";

$formats['zipcontent'] = "i5x/i5y/ii";

$world_bbox = array();
$world_bbox['l']['x'] = -20000000;
$world_bbox['l']['y'] = -20000000;
$world_bbox['h']['x'] = 20000000;
$world_bbox['h']['y'] = 20000000;

$fp = fopen($files['input'], 'r');
$sp = fopen($files['output'], 'w');

$files = array();
$offset = 0;

/**
 * Read through zipheaders
 * 
 */
while (!feof($fp)) {
	
	$buffer = fread($fp, 30);
	$tileinfo = unpack($formats['zipheader'], $buffer);
	
	if ($tileinfo['zipfnln'] <= 0)
		break;
	
	$filename = fread($fp, $tileinfo['zipfnln']);
	$x=0;
	$done=false;
	
	$r = $world_bbox;
	
	while (!$done) {
		$c['x'] = floor( ($r['l']['x'] + $r['h']['x'])/2 );
		$c['y'] = floor( ($r['l']['y'] + $r['h']['y'])/2 );
		
		switch($filename[$x]) {
		case 'a':
			$r['l']['x'] = $c['x'];
			$r['l']['y'] = $c['y'];
			break;
		case 'b':
			$r['h']['x'] = $c['x'];
			$r['l']['y'] = $c['y'];
			break;
		case 'c':
			$r['l']['x'] = $c['x'];
			$r['h']['y'] = $c['y'];
			break;
		case 'd':
			$r['h']['x'] = $c['x'];
			$r['h']['y'] = $c['y'];
			break;
		default:
			$done=true;
		}
		$x++;
	}
	
	$tilecontent = fread($fp, $tileinfo['zipsize']);
	
	/* Area inside box, save it! */
	if (contains_bbox($fetch_bbox, $r)) {
		#echo "In box. ";
		#echo $filename . " ";
		$zipheader = $buffer;
		#echo "\n";
		
	/* Area outside of box, set zipcontent=0 */
	} else {
		$tileinfo['zipmthd'] = $tileinfo['zipcrc'] = $tileinfo['zipsize'] = $tileinfo['zipuncmp'] = 0;
		#echo "Out of box";
		$zipheader = $tileinfo;
		$tilecontent = '';
		$zipheader = pack($formats['ziphpack'], 
							$tileinfo['ziplocsig'],
							$tileinfo['zipver'],
							$tileinfo['zipgenfld'],
							$tileinfo['zipmthd'],
							$tileinfo['ziptime'],
							$tileinfo['zipdate'],
							$tileinfo['zipcrc'],
							$tileinfo['zipsize'],
							$tileinfo['zipuncmp'],
							$tileinfo['zipfnln'],
							$tileinfo['zipxtraln']
							);
	}
	
	$put = $zipheader.$filename.$tilecontent;
	$files[$filename]['header'] = $tileinfo;
	$files[$filename]['size'] = strlen($put);
	
	$zipcd = array();
	$zipcd['zipcensig'] = 0x02014b50;
	$zipcd['zipcver'] 	= $tileinfo['zipver'];
	$zipcd['zipcos']	= 0x00;
	$zipcd['zipcvxt']	= 0x0a;
	$zipcd['zipcexos']	= 0x00;
	$zipcd['zipcflg']	= 0x00;
	$zipcd['zipcmthd']	= $tileinfo['zipmthd'];
	$zipcd['ziptim']	= $tileinfo['ziptime'];
	$zipcd['zipdat']	= $tileinfo['zipdate'];
	$zipcd['zipccrc']	= $tileinfo['zipcrc'];
	$zipcd['zipcsiz']	= $tileinfo['zipsize'];
	$zipcd['zipcunc']	= $tileinfo['zipuncmp'];
	$zipcd['zipcfnl']	= $tileinfo['zipfnln'];
	$zipcd['zipcxtl']	= 0x00;
	$zipcd['zipccml']	= 0x00;
	$zipcd['zipdsk']	= 0x00;
	$zipcd['zipint']	= 0x00;
	$zipcd['zipext']	= 0x00;
	$zipcd['zipofst']	= $offset;

	$zipcd_data .= pack($formats['zipcdpack'],
		$zipcd['zipcensig'],
		$zipcd['zipcver'],
		$zipcd['zipcos'],
		$zipcd['zipcvxt'],
		$zipcd['zipcexos'],
		$zipcd['zipcflg'],
		$zipcd['zipcmthd'],
		$zipcd['ziptim'],
		$zipcd['zipdat'],
		$zipcd['zipccrc'],
		$zipcd['zipcsiz'],
		$zipcd['zipcunc'],
		$zipcd['zipcfnl'],
		$zipcd['zipcxtl'],
		$zipcd['zipccml'],
		$zipcd['zipdsk'],
		$zipcd['zipint'],
		$zipcd['zipext'],
		$zipcd['zipofst']
	) . $filename;
		
	
	fwrite($sp, $put);
	$offset += strlen($put);
	
}

fwrite($sp, $zipcd_data);

fclose($fp);
fclose($sp);

?>

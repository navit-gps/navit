<?php
	set_time_limit(600);
	require_once("mapExtract.class.php");
	$bbox=split(',',urldecode($HTTP_GET_VARS['bbox']));
	if (count($bbox) == 4) {
		$mapextract = new mapExtract();
		$mapextract->setBbox($bbox[0], $bbox[1], $bbox[2], $bbox[3]);		
		$fp=fopen('php://output','w');
		$mapextract->setInput('../../planet.bin');
		$mapextract->setOutputFD($fp);
		if(isset($_SERVER['HTTP_USER_AGENT']) && strpos($_SERVER['HTTP_USER_AGENT'],'MSIE'))
			header('Content-Type: application/force-download');
		else
			header('Content-Type: application/octet-stream');
		$name='osm_bbox_';
		$name.=round($bbox[0],1) . ',' . round($bbox[1],1) . ','; 
		$name.=round($bbox[2],1) . ',' . round($bbox[3],1);
		$name.='.bin';
		header("Content-disposition: attachment; filename=\"$name\"");
		$error=$mapextract->process();
		if ($error) {
			header('Content-Type: text/plain');
			echo $error;
		}
		fclose($fp);
	} else {
		#echo "<pre>";
		#print_r($HTTP_HOST);
		#echo "</pre>";
		$areas=array(
			'Germany' => '5,47,16,55.1',
		);
		$url='http://' . $HTTP_HOST . $PHP_SELF;
		echo "Use: $url?bbox=bllon,bllat,trlon,trlat <br />\n";
		echo "<br />\n";
		while (list($area,$bbox)=each($areas)) {
			$urlf=$url . "?bbox=$bbox";
			echo "$area <a href='$urlf'>$urlf</a><br />\n";
		}
		
	}
?>

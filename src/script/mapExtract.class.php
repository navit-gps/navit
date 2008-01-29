<?php

class mapExtract {
  
    var $input_fd;
    var $output_fd;
    var $fetchBbox;
    
    var $formats;
    var $worldBbox;
    
    function mapExtract() {
        $formats = array();
        
        $formats['ziphpack'] = "lssssslLLSS";
        $formats['zipheader']  = "l" . "ziplocsig";    # Signature (is always the same)
        $formats['zipheader'] .= "/s" . "zipver";    # zip version needed
        $formats['zipheader'] .= "/s" . "zipgenfld";# type of os that generated the file
        $formats['zipheader'] .= "/s" . "zipmthd";    # 
        $formats['zipheader'] .= "/s" . "ziptime";    # time
        $formats['zipheader'] .= "/s" . "zipdate";    # date
        $formats['zipheader'] .= "/l" . "zipcrc";    # crc checksum
        $formats['zipheader'] .= "/L" . "zipsize";    # data size
        $formats['zipheader'] .= "/L" . "zipuncmp";    # uncompressed size
        $formats['zipheader'] .= "/S" . "zipfnln";    # length of filename
        $formats['zipheader'] .= "/S" . "zipxtraln";# length of extra data (always 0)
        
        $formats['zipcdpack'] = "iccccssssiIISSSSSII";
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
            "I" . "zipofst";
        
        $formats['zipeocpack'] = "iSSSSIIs";
        $formats['zipeoc'] = "".
            "i" . "zipesig/".
            "S" . "zipedsk/".
            "S" . "zipecen/".
            "S" . "zipenum/".
            "S" . "zipecenn/".
            "I" . "zipecsz/".
            "I" . "zipeofst/".
            "s" . "zipecoml/".
        
        $world_bbox = array();
        $world_bbox['l']['x'] = -20000000;
        $world_bbox['l']['y'] = -20000000;
        $world_bbox['h']['x'] = 20000000;
        $world_bbox['h']['y'] = 20000000;
        
        $this->formats = $formats;
        $this->worldBbox = $world_bbox;
    }
    
    
    function process() {
        
        if (!is_array($this->fetchBbox)) {
            return "Fetch box not set";
        }
        #
        if (!$this->input_fd) {
            return "No useable input set";
        }
        if (!$this->output_fd) {
            return "No useable output set";
        }
        $filecount = 0;
        $offset = 0;
        $zipcd_data = '';
        $report = array();
        
        /**
         * Read through zipheaders
         * 
         */
        for(;;) {
            $buffer = fread($this->input_fd, 30);
	    if (! strlen($buffer))
	    	break;
            $tileinfo = unpack($this->formats['zipheader'], $buffer);

            if ($tileinfo['ziplocsig'] != 0x4034b50)
	        break;
            
            if ($tileinfo['zipfnln'] <= 0)
                break;
            
            $filename = fread($this->input_fd, $tileinfo['zipfnln']);
            $done=false;
            
            $r = $this->worldBbox;
           
	    $len=strlen($filename); 
	    for ($i=0 ; $i < $len ; $i++) {
                $c['x'] = floor( ($r['l']['x'] + $r['h']['x'])/2 );
                $c['y'] = floor( ($r['l']['y'] + $r['h']['y'])/2 );
                
                switch($filename[$i]) {
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
		if ($done)
		    break;
            }
           #	print "zipsize=" . $tileinfo['zipsize']; 
            $tilecontent = fread($this->input_fd, $tileinfo['zipsize']);
           
	#	print "tile $filename"; 
            /* Area inside box, save it! */
            if ($this->contains_bbox($r)) {
                $report['added_areas']++;
                
                $zipheader = $buffer;
	#	print " in\n";
                
            /* Area outside of box, set zipcontent=0 */
            } else {
	#	print " out\n";
                $report['excluded_areas']++;
                
                $tileinfo['zipmthd'] = $tileinfo['zipcrc'] = $tileinfo
['zipsize'] = $tileinfo['zipuncmp'] = 0;
                $zipheader = $tileinfo;
                $tilecontent = '';
                $zipheader = pack($this->formats['ziphpack'], 
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
            
            /* Zip directory */
            $zipcd_data .= pack($this->formats['zipcdpack'],
                0x02014b50,
                $tileinfo['zipver'],
                0x00,
                0x0a,
                0x00,
                0x00,
                $tileinfo['zipmthd'],
                $tileinfo['ziptime'],
                $tileinfo['zipdate'],
                $tileinfo['zipcrc'],
                $tileinfo['zipsize'],
                $tileinfo['zipuncmp'],
                $tileinfo['zipfnln'],
                0x00,
                0x00,
                0x00,
                0x00,
                0x00,
                $offset
            ) . $filename;
            
            fwrite($this->output_fd, $put);
            $offset += strlen($put);
            $filecount += 1;
        }
        
        fwrite($this->output_fd, $zipcd_data);
        $ecsz = strlen($zipcd_data);
        
        /* Zip central directory */
        $zip_eoc = pack($this->formats['zipeocpack'], 
            0x06054b50,    #zipesig;
            0,             #zipedsk;
            0,            #zipecen;
            $filecount,    #zipenum;
            $filecount, #zipecenn;
            $ecsz,        #zipecsz;
            $offset,    #zipeofst;
            0             #zipecoml;
        );
        fwrite($this->output_fd, $zip_eoc);
        
        
        return null;
    }
    
    function setBbox($sx,$sy,$ex,$ey) {
        if ($ex<$sx)
            return false;
        if ($ey<$sy)
            return false;
        $this->fetchBbox = $this->getmercator($sx,$sy,$ex,$ey);
        return true;
    }

    function setInput($file) {
    	$this->input_fd=fopen($file,'r');
    }
    function setInputFD($fd) {
    	$this->input_fd=$fd;
    }
    
    function setOutput($file) {
    	$this->output_fd=fopen($file,'w');
    }

    function setOutputFD($fd) {
    	$this->output_fd=$fd;
    }
    
    
    function contains_bbox(&$r) {
        $c =& $this->fetchBbox;
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
    
}


?>

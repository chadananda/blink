<?php

 /*
  * PHP class to replace keywords in body text with links
  *
  *
 */


class blink {  

  // $links: array( array(
  //    kw     => 'human rights',
  //    url    => 'http://bahai.org/bahai-view-on-human-rights'
  //    weight => 57,
  //  ),) 
  
  // $potential_links: array( array(
  //   kw        => 'human rights',
  //   phrase    => 'Human Rights',
  //   url       => 'http://bahai.org/bahai-view-on-human-rights'
  //   positions => array(12,454,1346,73736),
  //   weight    => 57,
  // ),)
  
  // $new_links: array( array(
  //   keyword   => 'human rights',
  //   phrase    => 'Human Rights',
  //   url       => 'http://bahai.org/bahai-view-on-human-rights'  
  //   position  => 454,
  //   weight    => 57,
  // ),)

 function get_check_kw($kw) {
  //echo "kw: $kw <br>";
   $kw = preg_replace('#([\\\^\$\.\|\?\+\*\(\)\[\'])#', '\\\\$1', $kw);
   $kw = preg_replace('#^(\w)#', '\b\1', $kw);
   $kw = preg_replace('#(\w)$#', '\1\b', $kw);
   //echo "get_check_kw: $kw <br>";
   return $kw;
 }

 // Given a list of desired keyword links with weights
 //  this function will randomly select links (up to $maxlinks)
 //  with distribution exactly matching the keyword weight
 //  so a keyword weighted 10 will get twice the links as a 
 //  keyword weighted 5 (if all keywords are equally available).
 function markup_text(&$text, $links, $maxlinks=5, $class='', $style='') { 
 
 //echo "text: ".strlen($text).",maxlinks: $maxlinks, class: $class, style: $style, links: ". count($links) ."<br>";
 //exit;
  
   // cleanup a couple fields for Blink goals
 /*  foreach ($links as $key=>$link) {
     if ($link['local_weight']) $links[$key]['weight'] = $link['local_weight'];
     $links[$key]['kw_regex'] = $link['kw_regex'] ? $link['kw_regex'] : self::get_check_kw($link['kw']); 
   } */
   
// echo "<pre>". print_r($links, TRUE) . "</pre>";

   // loop through and gather all hit points
   foreach ($links as $key=>$link){
     // cleanup a couple fields
     if (isset($link['local_weight'])) $link['weight'] = $link['local_weight'];
     $link['kw_regex'] = $link['kw_regex'] ? $link['kw_regex'] : self::get_check_kw($link['kw']);  
   
     // if hit exists, store to potential_links list 
     $regex = '\'(?!((<.*?)|(<a.*?)))('. $link['kw_regex'] . ')(?!(([^<>]*?)>)|([^>]*?</a>))\'si'; 
     if (preg_match_all($regex, $text, $matches,  PREG_OFFSET_CAPTURE, 0)) {
         foreach ($matches[0] as $match) {
          $link['positions'][] = $match[1]; // record potential links
          $link['phrases'][] = $match[0]; // sometimes phrase will difer from keyword !!!
         }
         // we're reversing the two array to insert links from the bottom of the text up
         $link['positions'] = array_reverse($link['positions']);
         $link['phrases'] = array_reverse($link['phrases']); 
         
         $potential_links[] = $link;
     }
   }
     
  // echo "potential_links: <pre>". print_r($potential_links, TRUE) ."</pre>";
 // exit;
 
   // choose which links to use with randomized weighted distribution
   $new_links = array();  
   if (isset($potential_links)) while (count($potential_links) && (count($new_links) < $maxlinks)) { 
     // re-total link values each time
     $total_weight=0; 
     foreach ($potential_links as $link) $total_weight+= $link['weight'];   
     // pick a value from total range
     $pick = rand(0, $total_weight); $delta_weight=0; // $link is the pick
     foreach ($potential_links as $link_key=>$link) if ($pick <= ($delta_weight += $link['weight'])) break;
     //echo "<p> Picked link '{$link['kw']}' from ". count($potential_links) ." links, weighted {$link['weight']} with a random pick of {$pick}/{$total_weight} ";
     
     // walk through $potential_links position list to find a link that does not conflict with our existing $new_links
     $link['position'] = FALSE;
     foreach ($link['positions'] as $key=>$start) {
       $end = $start + strlen($link['phrases'][$key]);
       if (!self::position_conflict($start, $end, $new_links)) {
         $link['position'] = $start;
         $link['phrase'] = $link['phrases'][$key];
         unset($link['positions']);
         unset($link['phrases']);
         $new_links[] = $link; 
         break;
       } 
     } 
    unset($potential_links[$link_key]);
   }
  
  // sort array by 'position', highest first
  usort($new_links, array('blink', 'position_cmp'));  
          
  // loop through new_links (from high to low), applying links to text
  $class_attr = $class ? ' class="'. trim(stripslashes($class)) .'"' : '';
  $style_attr = $style ? ' style="'. trim(stripslashes($style)) .'"' : '';
  //drupal_set_message("Style: {$style}");
  foreach ($new_links as $link) { 
   $lnk = '<a href="'. $link['url'] .'"'. $class_attr . $style_attr .'>'. $link['phrase'] .'</a>';
   $text = substr_replace($text, $lnk, $link['position'], strlen($link['phrase']));
  } 
  
  return $new_links;
 }
 
 function position_cmp($a, $b) { 
   return $a['position'] < $b['position'];
 }
 
 function position_conflict($from, $to, $links){
  //echo "<p><b>position_conflict</b> $from, $to, <pre> ". print_r($links, TRUE) .'</pre>';
  foreach ($links as $link) {
    $begin = $link['position']; $end = $link['position']+strlen($link['phrase']);
    if (($from>=$begin && $from<=$end) || ($to>=$begin && $to<=$end))  return TRUE;  
  }
 }







 function markup_test(){
  $test_text = " 
<p><a href=''>New York City doesn't</a> have a friendliness problem as much as a communication problem. On the subway, the smart money is to pretend you're alone. Don't smile at strangers. Don't make eye contact. No one else exists. A cold commute, but maybe it's also why millions of strangers from pretty much every country on earth are able to ride belly to belly in a metal tube every day.</p>

<p>There's one day I remember vividly-this guy weeping on the subway, trying to suppress heartbreaking wails. I almost made a move toward him a couple of times, but I didn't. He got off the train as I debated with myself, but I just couldn't break the code of silence and stoicism. I sat there as another human being wept openly next to me. I sat like I was home alone watching Wheel of Fortune reruns. I could cry just thinking about it. But instead, I'm ramped up to make a difference.</p>

<p>Let's go out into the world and BE FRIENDLY. Make eye contact. Smile. Shake a hand. Hug a stranger. Break the silence. Let us know how it goes. And if you're as bold as I am, you'll let someone take a picture of your kindness in action to upload here.</p>

<p> Shackle (f) to a shackle with shackles.
";
  $links = array(
    array('kw' => 'New York City', 'weight' => 9, 'url' => 'http://bahai.org/bahai-view-on-new-york'),
    array('kw' => 'STOICISM',      'weight' => 8, 'url' => 'http://bahai.org/bahai-view-on-stoicism'),
    array('kw' => 'human rights',  'weight' => 7, 'url' => 'http://bahai.org/bahai-view-on-human-rights'),
    array('kw' => 'PROBLEM',       'weight' =>  6, 'url' => 'http://bahai.org/bahai-view-on-problems'),  
    array('kw' => 'SILENCE',       'weight' =>  5, 'url' => 'http://bahai.org/bahai-view-on-silence'),
    array('kw' => 'CoMmuniCaTion', 'weight' =>  4, 'url' => 'http://bahai.org/bahai-view-on-talking'),
    array('kw' => 'shackle',       'weight' => 90, 'url' => 'http://bahai.org/bahai-view-on-talking'),
  ); 
  
  echo '<h1> Keyword Markup Test </h1>'; 
  
  $maxlinks = 3;
  $class = 'test_class';
  $text = $test_text;  
  $new_links = self::markup_text($text, $links, $maxlinks, $class); 
  echo '<div style="border: 1px dashed gray; margin: 20px; padding:20px; padding-top:0">';
  echo   "<h3> markup_text(\$text, \$links, \$maxlinks={$maxlinks}, \$class='{$class}') </h3>";
  echo   "<p style='font-style:italic; color:gray'> ". count($new_links) ." matches </p>";
  echo   $text; 
  echo  '<pre> '. print_r($new_links, TRUE) .'</pre>';
  echo '</div>';  
 }
 
 
 






}

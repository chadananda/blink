<?php

// $Id$

/*
 * @file
 *
 */

if (!function_exists('db_fetch_array')) {
  function db_fetch_array($query) {
    if (is_object($query)) return $query->fetchAssoc();
  }
}
if (!function_exists('db_result')) {
  function db_result($query) {
    if (is_object($query)) return $query->fetchField();
  }
}
 

/*
 *  These will be pushed off into an include file 
 */ 
function blink_filter_keywords($text, $nid, $record_links = FALSE) {  
  if (!$text) return;
  if (!$nid) $record_links = FALSE;
  //  all links, adjust weighting as appropriate.
  $all_goals = blink_weighted_goals(); // should return a list indexed by GID so that we can remove items
  $cur_links = $record_links ? blink_current_page_links($nid) : array(); 
  $markup_links = array();   
  // pass in the old links with max number
  include_once(drupal_get_path('module', 'blink') . '/blink.class.php');
  $max_links = variable_get('blink_maximum_links_per_page', 6); 
  $link_class = variable_get('blink_link_class', '');
  if (count($cur_links)) $markup_links = blink::markup_text($text, $cur_links, $max_links, $link_class);  
  // if links back < total count, pass in again with linked links removed from all list and max being the difference
  if ($new_max = $max_links - count($markup_links)) {
    // remove curr_links from all_goals list (this is why lists are keyed by GID!!)
    foreach ($cur_links as $gid => $link) unset($all_goals[$gid]); 
    // now pass all links back to markup with reduced max_link target
    $additional_markup_links = blink::markup_text($text, $all_goals, $new_max, $link_class);
    // merge new links
    $markup_links = array_merge($markup_links, $additional_markup_links);
  }  
  // update stored links and add link style if this is not a teaser
  if ($record_links) blink_update_page_links($nid, $markup_links);    
  // return processed result with  link style
  
  //echo "got here"; exit;
  
  return $text;
}


 
function blink_absolute_goal_url($url, $override='') {
  global $base_url;
  // switch with override url if possible
  if ($override) $url = $override;
  // add base url to link only before using (don't store base_url in database)
  $url = strpos($url, '://') ? rtrim($url, '/') : rtrim($base_url, '/') . '/' . rtrim($url, '/');
  // shortens (externalizes) URL with 301 redirect before use, safe to use on any url, even already shortened
  if (module_exists('koc')) $url = koc_shorten_url($url);
  return $url;
}
 
 // TODO: get all the goal links and update table to populate max etc.  
function blink_weighted_goals($count=1000, $url='') {
  //drupal_set_message("blink_weighted_goals(\$count:$count, \$url:$url)");
  
  $result = array(); 
  if ($url) {
     $query = db_query_range('SELECT * FROM {blink_goals} WHERE weight>0 AND weight_local>0 AND url=:url'.
     ' ORDER BY weight_local DESC',
      1, $count, array(':url' => $url)); 
  } else {
    $query = db_query_range('SELECT * FROM {blink_goals} WHERE weight>0 AND weight_local>0'.
     ' ORDER BY weight_local DESC', 1, $count); 
  } 
  while ($goal = db_fetch_array($query)) {
    $url = isset($goal['short_url']) ? $goal['short_url'] : $goal['url'];
    $final_url =  blink_absolute_goal_url($url);
    $goal['url'] = $final_url;
    $result[$goal['gid']] = $goal;
  }
  
  //if ($url) drupal_set_message("<pre>".print_r($result, true) ."</pre>");
  
  return $result;
}

function blink_current_page_links($nid) {
  $result = array();
  $nid = (int) $nid;
  if ($nid) $ret = db_query('SELECT * FROM {blink_links} bll, {blink_goals} blg WHERE nid=:nid AND blg.gid = bll.gid', array(':nid' => $nid));
  while ($link = db_fetch_array($ret)) { 
    $url = isset($link['url_override']) ? $link['url_override'] : $link['url'];
    $link['url'] = $url; //??
    $result[$link['gid']] = $link;
  } 
  return $result;
}

function blink_update_page_links($nid, $new_links) {
  // note, gid's are not unique in the blink_links table but they are unique per page
  // ie. each page can have only one link per gid
  //
  // remove links from blink_links table that no longer exist
  $old_links = blink_current_page_links($nid);
  $new_gids = array();
  foreach ($new_links as $link) $new_gids[] = $link['gid']; // quick index
  if (count($old_links)) foreach ($old_links as $old) {
    if (!in_array($old['gid'], $new_gids)) {
      //db_query('DELETE FROM {blink_links} WHERE liid=:liid', array(':liid' => $old['liid']));
      db_delete('blink_links')->condition('liid', $old['liid'])->execute();
    }
  }
    
  // insert new link instances or update the old
  if (count($new_links)) foreach ($new_links as $new) {
    // if this link instance exists and is newer than the age of its goal record then leave it alone
    if ($new['liid'] && ($new['li_updated'] > $new['goal_updated'])) continue; 
    // this tests to see if updates are skipped correctly (when no change introduced)
    //drupal_set_message("Updating database for link instance: '{$new['kw']}'"); 
    
    //return; 
    $new['li_updated'] = time(); 
    // otherwise, create or update the record
    $new['nid'] = $nid;
    $new['link_uid'] = $new['link_uid'] ? $new['link_uid'] : uniqid('L', TRUE);
    $new['url_override'] = $new['short_url'];
    $new['page'] = isset($new['page']) ? $new['page'] : url('node/' . $nid, array('absolute' => TRUE));
    $liid_key = isset($new['liid']) ? array('liid') : array();
    
    $link = array_intersect_key($new, array('liid'=>'','link_uid'=>'','gid'=>'','nid'=>'',
     'page'=>'', 'url_override'=>'', 'li_updated'=>'')); 
    //drupal_set_message("<pre>". print_r($link, true) ."</pre>");
    //return;
    if (is_array($link)) {
       drupal_write_record('blink_links', $link, $liid_key); 
       //drupal_set_message("Writing record: <pre>". print_r($link, true) ."</pre>");
       // update or insert
    }
  } 
}

 

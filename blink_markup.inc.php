<?php


 

/*
 *  These will be pushed off into an include file 
 */ 
function blink_filter_keywords($text, $nid, $is_teaser = FALSE) {
  // drupal_set_message('Applying filter to node: '. $nid);
 
  //  all links, adjust weighting as appropriate.
  $all_goals = blink_all_weighted_goals(); // should return a list indexed by GID so that we can remove items
  $cur_links = blink_current_page_links($nid); // this list is indexed by GID also
  $markup_links = array();

  // drupal_set_message("All Links: <pre>".print_r($all_goals, true)."</pre>");
  //drupal_set_message("Current Links: <pre>".print_r($cur_links, true)."</pre>");
   
  // pass in the old links with max number
  include_once(drupal_get_path('module', 'blink') .'/blink.class.php');
  $max_links = variable_get('blink_maximum_links_per_page', 6);
  if (!$nid) $max_links = 50;
  // set a link_class value if none has been chosed already
  $link_class = variable_get('blink_link_class', '');
  if (count($cur_links)) $markup_links = blink::markup_text($text, $cur_links, $max_links, $link_class); 
  // drupal_set_message("Markup Links: <pre>".print_r($markup_links, true)."</pre>");

  // if links back < total count, pass in again with linked links removed from all list and max being the difference
  if ($new_max = $max_links - count($markup_links)) {
    // remove curr_links from all_goals list (this is why lists are keyed by GID!!)
    foreach ($cur_links as $gid=>$link) unset($all_goals[$gid]);

    // now pass all links back to markup with reduced max_link target
    $additional_markup_links = blink::markup_text($text, $all_goals, $new_max, $link_class);
    // merge new links
    $markup_links = array_merge($markup_links, $additional_markup_links);
  }

  // update stored links and add link style if this is not a teaser
  if (!$is_teaser) {
    blink_update_page_links($nid, $markup_links);
    // $text = blink_link_style() . $text; // replace with file based solution
  }
  
  // return processed result with  link style
  return $text;
}

// TODO: check the node against exclusion list (node types and pattern)
function blink_node_selected_for_markup($node) {
  $deselected_types = variable_get('blink_exclude_types', array());
  if (is_array($deselected_types) && in_array($node->type, $deselected_types)) return FALSE;
  
  // TODO: return FALSE if path matches exclusion path list

  //drupal_set_message("This '{$node->type}' node is being marked up");
  return TRUE;
}
 
function blink_absolute_goal_url($url, $override='') {
  global $base_url;
  // switch with override url if possible
  if ($override) $url = $override;
  // add base url to link only before using (don't store base_url in database)
  $url = strpos($url, '://') ? rtrim($url, '/') : rtrim($base_url, '/') .'/'. rtrim($url, '/');
  // shortens (externalizes) URL with 301 redirect before use, safe to use on any url, even already shortened
  if (module_exists('koc')) $url = koc_shorten_url($url);
  return $url;
}
 
 // TODO: get all the goal links and update table to populate max etc.  
function blink_all_weighted_goals() {
  $result = array(); 
  $ret = db_query('SELECT * FROM {blink_goals} WHERE weight > 0 AND weight_local > 0', $nid);
  while ($goal = db_fetch_array($ret)) {
    $url =  blink_absolute_goal_url($goal['url'], $goal['url_override']);
    $goal['url'] = $url;
    $result[$goal['gid']] = $goal;
  }
  return $result;
}

function blink_current_page_links($nid) {
  $result = array();
  $nid = (int) $nid;
  if ($nid) $ret = db_query('SELECT * FROM {blink_links} bll, {blink_goals} blg WHERE nid=%d AND blg.gid = bll.gid', $nid);
  while ($link = db_fetch_array($ret)) {
    $link['url'] = blink_absolute_goal_url($link['url'], $link['url_override']);
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
  if (count($old_links)) {
    foreach ($old_links as $old) {
      if (!in_array($old['gid'], $new_gids)) {
        db_query('DELETE FROM {blink_links} WHERE liid=%d', $old['liid']);
      }
    }
  }
    
  // insert new link instances or update the old
  if (count($new_links)) foreach ($new_links as $new) {
    // if this link instance exists and is newer than the age of its goal record then leave it alone
    if ($new['liid'] && ($new['li_updated'] > $new['goal_updated'])) continue;

    // this tests to see if updates are skipped correctly (when no change introduced)
    // drupal_set_message("Updating database for link instance: '{$new['kw']}'");

    $new['li_updated'] = time(); 
    // otherwise, create or update the record
    $new['nid'] = $nid;
    $new['link_uid'] = $new['link_uid'] ? $new['link_uid'] : uniqid('L', TRUE);
    $new['page'] = $new['page'] ? $new['page'] : url('node/'. $nid, array('absolute'=>TRUE));
    $liid_key = (int)$new['liid'] ? array('liid') : NULL;
    drupal_write_record('blink_links', $new, $liid_key); // update or insert
  } 
}

/*
function blink_link_style() {
  if (($class = variable_get('blink_link_class', '')) &&($style = variable_get('blink_link_style', ''))) { 
    $style_hover = variable_get('blink_link_style_hover', '');
    return "<style>\n    a.{$class} {{$style}} \n    a.{$class}:hover {{$style_hover}} \n</style>";
  }
}
 *
 */

<?php
// $Id$

/**
 * @file
 * blink_update.inc.php
 *
 * Code responsible for updating the link goals from a remote linklist server
 */


/*
 * Update Goals or Request key 
 * 
 * $server : {blink_servers} record
 * $blink_version : double : version of blink module
 */
function xmlc_blink_update_goals($server, $blink_version) {  
  // get this server and test
  global $base_url;
   if (!($this_server = parse_url($base_url, PHP_URL_HOST))) return FALSE;
    else $this_server = parse_url($base_url, PHP_URL_SCHEME) .'://'. $this_server;
  if (!valid_url($this_server)) return FALSE;
  // get email and test
  if (!valid_email_address($email = variable_get('site_mail', ''))) return FALSE; // what if site_mail is invalid?
  include_once(drupal_get_path('module', 'blink') .'/blink_admin.inc.php');
   if (!$server_url = blink_get_server_RPC_url($server['server'])) return FALSE;

  // if no uniqid, create one -- is this really necessary?  
  if (!$server['server_key']) $server['server_key'] = uniqid('b', TRUE);
 
  // update last attempted update date
  $server['last_attempted_update'] = time();
 
  // gather link instances if we're not in pending or unregistered state
  if (!in_array($server['status'], array('unregistered', 'pending_approval'))) $link_instances = blink_fetch_link_instances($server['blsid']);

  drupal_set_message(t("Sending %count link instances to server '%server'.",
      array('%count' => count($link_instances), '%server' => $server['server'])));

  // XML-RPC, try up to twice
  $response = (array) xmlrpc($server_url, 'linklistMaintainer.getLinklist', $this_server, $email, $server['server_key'], $blink_version, $link_instances);

  if ($error = xmlrpc_error()) {
    drupal_set_message("Remote site gave an error: {$error->message} ({$error->code})");
    return;
  }

  // update link goals list if one was passed back successfully
  if ($response['success']) {
    if (count($response['linklist'])>0) {
      drupal_set_message(t("Received %count link goals from server '%server', updating goals.",
          array('%count' => count($response['linklist']), '%server' => $server['server'])));
      // got new goals, store goals list (including calculating local weights and server max weight)
      blink_update_goals($response['linklist'], $server);
      // set status to 'successful_update'
      $server['status'] = 'successful_update';
      $server['last_successful_update'] = time();
    } else $server['status'] = 'failed_update';
  }
 
  // update last_updated even if we received no response
  drupal_write_record('blink_servers', $server, array('blsid'));
}

/*
 * $linklist is an updated list of goals
 * $server is the record of this server
 */
function blink_update_goals($linklist, $server) {
  //drupal_set_message('Blink got a linklist, updating '. count($linklist) . ' items.');
  $old_goals = array();

  // get all old goals in goal_uid keyed array for fast lookup
  $ret = db_query('SELECT * FROM {blink_goals} WHERE blsid="%d"', $server['blsid']);
  while ($goal = db_fetch_array($ret)) $old_goals[$goal['goal_uid']] = $goal;
  
 // drupal_set_message("Linklist: <pre>". print_r($linklist, TRUE) ."</pre>");
  //drupal_set_message("Current list of goals: <pre>". print_r($old_goals, TRUE) ."</pre>");

  // DEACTIVATE OLD
  // loop through old goals, if not found in new list, set as inactive (weight =0)
  foreach ($old_goals as $uid => $old) if (!$linklist[$uid]) {
    $old['weight'] = 0;
    drupal_write_record('blink_goals', $old, array('gid'));
  }

  // UPDATE and INSERT
  // look through new goals, if not found in old links assign gid (or insert if new)
  if (is_array($linklist)) {
    include_once(drupal_get_path('module', 'blink') . '/blink.class.php');
      foreach ($linklist as $uid => $new) {
      $old = $old_goals[$uid];
      // skip goal update completely if new goal matches old goal in kw, url and weight
      if (($old['kw']==$new['kw']) && ($old['url']==$new['url']) && ($old['weight']==$new['weight'])) continue;
      // update timestamp only if either kw or url have changed
      if (($old['kw']==$new['kw']) && ($old['url']==$new['url'])) $new['goal_updated'] = $old['goal_updated'];
       else $new['goal_updated'] = time();
      // insert or update record
      $new['blsid'] = $server['blsid'];
      $new['gid'] = $old['gid'];
      $new['kw_regex'] = ($old['kw_regex'] && ($old['kw'] == $new['kw'])) ? $old['kw_regex'] : blink::get_check_kw($new['kw']);
      $primary_key = $old ? array('gid') : NULL; // no gid forces INSERT
      //drupal_set_message("drupal_write_record: table: 'blink_goals', \$primary_key: '{$primary_key}', record: <pre>". print_r($linklist[$goal_uid], TRUE) ."</pre>");
      // if ($old)  drupal_set_message('Updating remote goal: '. $new['kw'] .' -> '. $new['url'] .', weight: '. $new['weight']);
      // else   drupal_set_message('Adding new remote goal: '. $new['kw'] .' -> '. $new['url'] .', weight: '. $new['weight']);
      // drupal_set_message("New item: <pre>". print_r($new, TRUE) ."</pre>");
      drupal_write_record('blink_goals', $new, $primary_key);
    }
  }

 // update all local weights
 blink_update_all_goal_localweights();
}

function blink_update_all_goal_localweights() {
  drupal_set_message("Updating all localweight values");
  $scary_sql = "UPDATE {blink_goals} g,
             ( SELECT g1.blsid,  g1.server_weight / g2.sum_weight sv_weight
               FROM {blink_servers} g1,
               (SELECT blsid, sum(server_weight)  sum_weight
                FROM {blink_servers}
                GROUP BY blsid) g2
                WHERE g1.blsid = g2.blsid
             ) svr,
             ( SELECT g1.gid, g1.blsid, g1.weight / g2.sum_weight goal_weight
               FROM {blink_goals} g1,
               (SELECT blsid, sum(weight) sum_weight
                FROM {blink_goals}
                GROUP BY blsid) g2
                WHERE g1.blsid = g2.blsid
              ) gl
        SET   weight_local = svr.sv_weight * gl.goal_weight * 1000
        WHERE g.blsid = gl.blsid
        AND   g.gid =  gl.gid
        AND   g.blsid = svr.blsid";
   if (!db_query($scary_sql)) drupal_set_message("Localweight query failed!", 'warning');
   return;
  /* gee, I hate to throw away elegant code... even when one has the opportunity to break MVC by stuffing logic in SQL...
  // gather up list of all servers
  $ret = db_query('SELECT * FROM {blink_servers}');
  while ($server = db_fetch_array($ret)) $servers[$server['blsid']] = $server;
 
  foreach ($servers as $bslid => $server) {  
    // server weight ratio is a ratio of this server's assigned weight to the total of all server's weights (plus one for local links)
    $servers_weight_total = db_result(db_query('SELECT SUM(weight)+1 FROM {blink_servers}'));
    $server_weight_ratio = $server['weight'] / $servers_weight_total;
    // get total goal weight
    $goal_weight_total = db_result(db_query('SELECT SUM(weight) FROM {blink_goals} WHERE blsid=%d', $blsid)); 

    // gather and adjust local weights for all goals from this server
    $ret = db_query('SELECT * FROM {blink_goals} WHERE blsid=%d', $blsid);
    while ($goal = db_fetch_array($ret)) $goals[$goal['gid']] = $goal;
    
    // loop through all goals calculating new localweight
    foreach ($goals as $gid=>$goal) {
      // first equalize this goal for this server so that all server's goal weights would total the same
      // for example a server with goal weights 1/2/1/2 would have the same weight as a server with goal weights 5/10/5/10 
      $relative_goalweight = $goal['weight'] / $goal_weight_total;  
      // now adjust local weight by the relative weight given to this particular server
      // for example, a server weighted 2 with links 1/2/1/2 would have double the weight of a server weighted 1 with links 5/10/5/10
      $localweight_float = $relative_goalweight * $server_weight_ratio; 
      // now multiply by 1000 and convert to integer for fast processing
      $goal['weight_local'] = round($localweight_float * 1000);
      // save updated record
      drupal_write_record('blink_goals', $goal, array('gid'));
    }
  }
 */
}


 
 
/*
 * Gather link instances for updating remote server or omit blisd for local links
 */ 
function blink_fetch_link_instances($blsid=0) {
  $ret = db_query('SELECT * FROM {blink_links} bll, {blink_goals} blg WHERE blsid=%d AND blg.gid = bll.gid', $blsid);
  while ($link = db_fetch_array($ret)) {
    $links[$link['link_uid']] = array(
      'link_uid'    => $link['link_uid'],
      'goal_uid'    => $link['goal_uid'],
      'page'        => $link['page'],
      'li_updated'  => $link['li_updated'],
    );    
  }
  return $links;
}
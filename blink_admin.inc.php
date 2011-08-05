<?php
// $Id$

/**
 * @file
 * Blink client linking module
 *
 * Administrative forms and report pages
 */

      // query for all local link goals with stats
/*      
 SELECT bg.*, bl.gid, count(bl.liid) lin_count FROM blink_goals bg, blink_links bl
 WHERE blsid=x AND bg.gid=bl.gid
 GROUP by bl.liid     
      
 SELECT bg.*, bl.gid, count(bl.liid) lin_count FROM blink_goals bg, blink_links bl
 WHERE blsid=x AND bg.gid=bl.gid
 GROUP by bl.liid
      // foreach 
 */     
      
function blink_link_report() {
  drupal_add_css(drupal_get_path('module', 'blink') .'/blink_form.css', 'module', 'all', FALSE);
  drupal_add_js(drupal_get_path('module', 'blink') . '/blink_form.js'); 
  
  // local links
    // Local link goals by weight (12 goals, 365 links on 87 pages)
      // kw, url, weight, # links, # pages
      // ..  
  // gather up all goals for this server
  $local_blsid = db_result(db_query('SELECT blsid FROM {blink_servers} WHERE server="local"'));
  $ret = db_query("SELECT g.*, COUNT(*) link_count FROM {blink_goals} g, {blink_links} l ".
    "WHERE g.gid=l.gid AND g.blsid=%d GROUP by g.gid ORDER BY g.weight DESC", $local_blsid);
  while ($goal = db_fetch_array($ret)) $goals[] = $goal;
  if (count($goals)) {
    // for each site
    $link_count = 0; foreach ($goals as $goal) $link_count += $goal['link_count'];
    $page_count = db_result(db_query("SELECT COUNT(DISTINCT l.nid) page_count FROM {blink_goals} g, {blink_links} l ".
        "WHERE g.gid=l.gid AND g.blsid=%d", $local_blsid));
    $goals_list = "<ul class='goals'> \n";
    // for each goal in this site
    foreach ($goals as $goal) {
      // kw, url, weight, # links, # pages
      $goals_list .= "  <li> <u>{$goal['kw']}</u> -> {$goal['url']}: {$goal['link_count']} links. </li> \n";
    }
    $local_report .= $goals_list .'</ul></li>';
    // main header for local server links
    $totals = '('. count($goals) ." goals met with {$link_count} links on {$page_count} pages.)";
    $local_report = "<h2 class='local_server'> Local Links: {$totals} </h2> <ol class='goal_list'>". $local_report .'</ol>';
  }
  
  
  // remote links: (89 goals, 1,250 links on 240 page)
    // example.com goals: (site_weight, 25 goals, 450 links on 92 pages)
      // kw, url, weight, # links
      // .. 
    // example2.com goals: (site_weight, 25 goals, 450 links on 92 pages)
      // kw, url, weight, # links
      // ..
  $ret = db_query("SELECT blsid,server,server_weight FROM {blink_servers} WHERE blsid<>%d ORDER BY server_weight DESC", $local_blsid);
  //    $ret = db_query("SELECT blsid,server,server_weight FROM {blink_servers} ORDER BY server_weight DESC");
  while ($server = db_fetch_array($ret)) $servers[] = $server;
  if (count($servers)) {
    foreach ($servers as $server) {
      // gather up all goals for this server
      $goals = array();
      $ret = db_query("SELECT g.*, COUNT(*) link_count FROM {blink_goals} g, {blink_links} l ".
        "WHERE g.gid=l.gid AND g.blsid=%d GROUP by g.gid ORDER BY g.weight DESC", $server['blsid']);
      while ($goal = db_fetch_array($ret)) $goals[] = $goal;
      if (count($goals)) {
        // for each site
        $link_count = 0; foreach ($goals as $goal) $link_count += $goal['link_count'];
        $page_count = db_result(db_query("SELECT COUNT(DISTINCT l.nid) page_count FROM {blink_goals} g, {blink_links} l ".
            "WHERE g.gid=l.gid AND g.blsid=%d", $server['blsid']));
        // example.com goals: (site_weight, 25 goals, 450 links on 92 pages)
        $servers_report .= "<li><h3 class='remote_server'> <b>". l($server['server'])  ."</b> goals: (". count($goals) ." goals, ".
          "{$link_count} links on {$page_count} pages) </h3>";
        $goals_list = "<ul class='goals'> \n";
        // for each goal in this site
        foreach ($goals as $goal) {
          // kw, url, weight, # links, # pages
          $goals_list .= "  <li> <u>{$goal['kw']}</u> -> {$goal['url']}: {$goal['link_count']} links. </li> \n";
        }
        $servers_report .= $goals_list .'</ul></li>';
        $total_links += $link_count;
        $total_goals += count($goals);
        $total_pages += $page_count;
      }
    }
    // main header for remote server links
    if (count($servers)>0) $totals = '('.count($servers) ." servers with {$total_goals} goals met, {$total_links} links on {$total_pages} pages.)";
    $servers_report = "<h2 class='remote_servers'> Remote Links: {$totals} </h2> <ol class='goal_list'>". $servers_report .'</ol>';
  }  
      
  return $local_report . $servers_report;
}

function blink_force_update_now() {
 /*  // for local testing, override server_key with key matching linklist variable
  $server['server_key'] = variable_get('linklist_maintainer_test_key', user_password(40));
  variable_set('linklist_maintainer_test_key', $server['server_key']);
  drupal_set_message('blink_update.inc line 33-36: Set server_key = varible "linklist_maintainer_test_key". <br> Make sure to remove this code after testing', 'warning');
 */
  
  // update goals and links whenever the admin screen is viewed -- for development
  blink_update_all_servers();
  drupal_goto('admin/settings/blink/settings');   
}

function blink_admin_settings_form() {  
  drupal_add_css(drupal_get_path('module', 'blink') .'/blink_form.css', 'module', 'all', FALSE);
  drupal_add_js(drupal_get_path('module', 'blink') . '/blink_form.js');  
  
  // list of local links
  blink_form_local_links($form);

  // list of remote servers plus one extra field row to add a server
  blink_form_remote_servers($form); 

  // Site Settings
  blink_form_site_settings($form);

  // page selections
  blink_form_page_selections($form); 

  $form['#validate'][] = 'blink_admin_settings_validate';
  $form['#submit'][] = 'blink_admin_settings_submit';

  return system_settings_form($form);
}

function blink_form_remote_servers(&$form) {
  // list of remote servers plus one extra field row to add a server
  // ***************************************************************
  $form['remote_servers']  = array(
    '#type'         => 'fieldset',
    '#title'        => t('Link Goal Servers'),
    '#description'  => t('Warning: only add link servers that you trust implicitly!'),
    '#collapsible'  => TRUE,
    '#collapsed'    => TRUE,
  );
  // servers table
  $table_header = '<table class="blink_servers">'.
    '<tr> <th></th> <th>'. t('Server') .'</th><th>'. t('Weight') .'</th><th>'. t('Remove').'</th> </tr>';
  $form['remote_servers']['markup_servers_table'] = array('#value' => $table_header,);
  // weight of local links
  $local_server = db_fetch_array(db_query("SELECT * FROM {blink_servers} WHERE server='local'")); 
  $form['remote_servers']['blink_local_links_weight'] = array(
      '#type' => 'textfield',
      '#required' => TRUE,
      '#default_value' => $local_server['server_weight'],
      '#size' => 3,
      '#prefix' => '<tr><td></td><td>'. t('Local link goals') .'</td><td>',
      '#suffix' => '</td></td><td></tr>',
  );
  // re-gather the defaults if the form has been reset
  if (variable_get('blink_defaults_reset', FALSE)) blink_get_default_servers();
    // list of remote servers, ordered by weight
  if ($ret = db_query('SELECT * FROM {blink_servers} WHERE server<>"local"')) {
    while ($server = db_fetch_array($ret)) $servers[] = $server; 
    if ($servers) {
      usort($servers, 'blink_sort_by_server_weight'); 
      foreach ($servers as $key=>$server) blink_server_form_add_server_field($form, $key, $server);
      // an extra, empty row to add another server
      blink_server_form_add_server_field($form, count($servers));
    }
  }
  // close html table
  $form['remote_servers']['markup_servers_table_end'] = array('#value' => '</table>',);
}

function blink_form_site_settings(&$form) {
   // Site Settings
  // ***************************************************************
  $form['site_settings']  = array(
    '#type'         => 'fieldset',
    '#title'        => t('Site Settings'),
    '#collapsible'  => TRUE,
    '#collapsed'    => TRUE,
  );
  // max page links
  $form['site_settings']['blink_maximum_links_per_page'] = array(
    '#type' => 'select',
    '#title' => t('Max links'),
    '#default_value' => variable_get('blink_maximum_links_per_page', 6),
    '#options' => array(1=>1,2=>2,3=>3,4=>4,5=>5,6=>6,7=>7,8=>8,9=>9,10=>10,20=>20),
    '#description' => t('Maximum number of links to be added to any given page'),
  );

  $form['site_settings']['link_class']  = array(
    '#type'         => 'fieldset',
    '#title'        => t('Link Markup'),
    '#collapsible'  => TRUE,
    '#collapsed'    => TRUE,
  );

  // class to use for links
  $form['site_settings']['link_class']['blink_link_class'] = array(
    '#type' => 'textfield',
    '#title' => t('Blink link class'),
    '#default_value' => variable_get('blink_link_class', ''),
    '#description' => t('Optional CSS class to assign to links generated by Blink'),
  );

  $form['site_settings']['link_class']['blink_link_style'] = array(
    '#type' => 'textfield',
    '#title' => t('Blink link style'),
    '#default_value' => variable_get('blink_link_style', 'text-decoration:none; border-bottom:1px dashed silver;'),
    '#description' => t('Optional CSS style to be applied to links'),
  );

  $form['site_settings']['link_class']['blink_link_style_hover'] = array(
    '#type' => 'textfield',
    '#title' => t('Blink link hover style'),
    '#default_value' => variable_get('blink_link_style_hover', 'background:#FF9; border-bottom:1px solid;'),
    '#description' => t('Optional CSS style to be applied to links on hover'),
  );

}

function blink_form_page_selections(&$form) {
  // page selections
  // ***************************************************************
  $form['page_selections']  = array(
    '#type'         => 'fieldset',
    '#title'        => t('Page Exclusions'),
    '#collapsible'  => TRUE,
    '#collapsed'    => TRUE,
  );

  // exclude content types
  $types = array_keys(node_get_types());
  foreach ($types as $type) $options[$type]=$type;

  $form['page_selections']['blink_exclude_types'] = array(
    '#type'     => 'checkboxes',
    '#title'    => t('Exclude content types'),
    '#default_value' => variable_get('blink_exclude_types', array_keys($types)), // list of array keys matching node types array
    '#options'  =>  $options,
    '#multiple'  => TRUE,
    '#description'  => t('Links will NOT be created on checked content types s'),
  );

  // exclude pattern
  $form['page_selections']['blink_exclude_paths'] = array(
    '#type'     => 'textarea',
    '#title'    => t('Exclude paths'),
    '#default_value' => variable_get('blink_exclude_paths', ''),
    '#description'  => t('Links will not be created for paths entered here. Enter one path per line. <br />'.
                         "'*' is a wildcard character for any character except '/'"),
  );
}
 
function blink_form_local_links(&$form) {
  // list of local links plus an extra to add a new link
  // ***************************************************************
  // list of local goals, ordered by weight
  $goals = array();
  $local_blsid = db_result(db_query('SELECT blsid FROM {blink_servers} WHERE server="local"'));
  $ret = db_query("SELECT * FROM {blink_goals} WHERE blsid=%d ORDER BY weight DESC", $local_blsid);
  while ($goal = db_fetch_array($ret)) $goals[] = $goal;
  $total_count = count($goals);

  // Local Links 
  $form['local_goals']  = array(
    '#type'         => 'fieldset',
    '#title'        => t('Local Link Goals'),
    '#collapsible'  => TRUE,
    '#collapsed'    => (!$total_count), // collapse if none exist
  ); 
  
  // first, pull out the blink goals 
   
  foreach ($goals as $key=>$goal) $goals[$key]['num'] = $key; // store the num with each 
  foreach ($goals as $goal) $goal_groups[$goal['goal_source']][] = $goal;
  //drupal_set_message('Goals: <pre>'. print_r($goal_groups, TRUE) . "</pre>");
  //if (!$goal_groups) return;
  
  // first, display Blink local links plus an add row (since blink links can be added manually)
  if ($goal_groups) foreach ($goal_groups as $group_name=>$group) if ($group_name=='blink') {
     blink_local_form_add_goal_group($form, $group, $group_name, $total_count);
     $blink_links_found = TRUE;
     break;
  }
  // if no blink links found, display the section with 'add' row anyway
  if (!$blink_links_found) blink_local_form_add_goal_group($form, '', 'blink', $total_count);
  
  // now display any other groups of links
  if ($goal_groups) foreach ($goal_groups as $group_name=>$group) if ($group_name!='blink') blink_local_form_add_goal_group($form, $group, $group_name);  
  
  /*
  // servers table
  $table_header = '<table class="blink_local_goals">' .'<tr> <th></th> <th>'. t('Keyword or Phrase') .'</th> <th>'. t('Target URL') .'</th> <th>'.
    t('Weight') .'</th><th>'. t('Remove')  .'</th> </tr>'; 
    
  $form['local_goals']['markup_local_goals_table'] = array('#value' => $table_header,);
  

  // fetch one field for each exiting local link 
  if (count($goals)) foreach ($goals as $key=>$goal) blink_local_form_add_goal_field($form, $key, $goal);
  // an extra, empty row to add another server
  blink_local_form_add_goal_field($form, count($goals));
  
  // close html table
  $form['local_goals']['markup_servers_table_end'] = array('#value' => '</table>',);
  */
}  

function blink_local_form_add_goal_group(&$form, $group, $group_name, $total_count=0) { 

  // Local Links 
  $form['local_goals'][$group_name]  = array(
    '#type'         => 'fieldset',
    '#title'        => $group_name,
    '#collapsible'  => TRUE,
    '#collapsed'    => ($group_name!='blink'),  
  ); 

  // servers table
  $table_header = '<table class="blink_local_goals">' .'<tr> <th></th> <th>'. t('Keyword or Phrase') .'</th> <th>'. t('Target URL') .'</th> <th>'.
    t('Weight') .'</th><th>'. t('Remove')  .'</th> </tr>'; 
    
  $form['local_goals'][$group_name]['markup_local_goals_table'] = array('#value' => $table_header,);
  

  // fetch one field for each exiting local link 
  if (is_array($group) && count($group)) foreach ($group as $goal) blink_local_form_add_goal_field($form, $goal['num'], $goal, $group_name); 
  if ($group_name == 'blink') blink_local_form_add_goal_field($form, $total_count, '', $group_name);
  // an extra, empty row to add another server
  
  // close html table
  $form['local_goals'][$group_name]['markup_servers_table_end'] = array('#value' => '</table>',);

}

function blink_local_form_add_goal_field(&$form, $num, $goal, $group_name) {
  $exists = is_array($goal); 
  $weight = $goal['weight'] ? $goal['weight'] : 1;
  $blsid  = $goal['blsid']; 
  if (!$exists) $connect_image = '<img src="/'. drupal_get_path('module', 'blink') . '/add.png' . '" title="Add new server" width="16" />'; 
 
  $form['local_goals'][$group_name]["markup_field_servers_row_{$num}"] = array(
    '#value' => "<tr>",
  );
  $form['local_goals'][$group_name]["markup_field_servers_row_header_{$num}"] = array(
     '#value' => '<td>'. $connect_image  .'</td>',
  );

  if ($exists) $form['local_goals'][$group_name]["blink_local_goal_gid_{$num}"] = array(
    '#type' => 'hidden',
    '#value' => $goal['gid'],
  );
  
  // if ($exists && !$goal['gid']) drupal_set_message('No goal id found in goal array: <pre>'.print_r($goal, TRUE).'</pre>', 'warning');
  

  $form['local_goals'][$group_name]["blink_local_goal_kw_{$num}"] = array(
    '#type' =>  $exists ? 'hidden' : 'textfield',
    '#title' => t(''),
    '#description' =>  t(''),
    '#required' => FALSE,
    '#default_value' => $goal['kw'],
    '#size' => 15,
    '#prefix' => '<td>'. $goal['kw'],
    '#suffix' => '</td>',
  );
  $form['local_goals'][$group_name]["blink_local_goal_url_{$num}"] = array(
    '#type' =>  $exists ? 'hidden' : 'textfield',
    '#title' => t(''),
    '#description' =>  t(''),
    '#required' => FALSE,
    '#default_value' => $goal['url'],
    '#size' => 35,
    '#prefix' => '<td>'. l($goal['url'], $goal['url'], array('attributes' => array('target' => '_blank'))),
    '#suffix' => '</td>',
  );

  $form['local_goals'][$group_name]["blink_local_goal_weight_{$num}"] = array(
    '#type' => 'textfield',
    '#title' => t(''),
    '#description' =>  t(""),
    '#required' => FALSE,
    '#default_value' => $weight,
    '#size' => 3,
    '#prefix' => '<td>',
    '#suffix' => '</td>',
  );

  if ($exists) {
    $form['local_goals'][$group_name]["blink_local_goal_remove_{$num}"] = array(
      '#type' => 'checkbox',
      '#title' => t(''),
      '#description' =>  t(""),
      '#required' => FALSE,
      '#default_value' => FALSE,
      '#prefix' => '<td>',
      '#suffix' => '</td>',
    ); 
   } else $form['local_goals'][$group_name]["blink_local_goal_remove_{$num}"] = array('#value' => '<td></td>',);
      
  
  $form['local_goals'][$group_name]["markup_field_servers_row_{$num}_end"] = array(
    '#value' => '</tr>',
  );

} 

function blink_server_form_add_server_field(&$form, $num, $server_fields='') {
  if (is_array($server_fields)) {
    $weight = $server_fields['server_weight'];
    $server = $server_fields['server'];
    $blsid  = $server_fields['blsid'];
    
    $can_connect = blink_get_server_RPC_url($server);

    if ($can_connect) $connect_image = '<img src="/'. drupal_get_path('module', 'blink') . '/checked.png' . '" title="Connection OK" width="15" />';
     else $connect_image = '<img src="/'. drupal_get_path('module', 'blink') . '/failed.png' . '" title="Connection Failed!" width="15" />';
  } else {
    $connect_image = '<img src="/'. drupal_get_path('module', 'blink') . '/add.png' . '" title="Add new server" width="16" />';
  }
 
  $form['remote_servers']["markup_field_servers_row_{$num}"] = array(
    '#value' => "<tr>",
  );
  $form['remote_servers']["markup_field_servers_row_header_{$num}"] = array(
     '#value' => '<td>'. $connect_image  .'</td>',
  );

  $form['remote_servers']["blink_server_{$num}"] = array(
    '#type' => $server ? 'hidden' : 'textfield',
    '#title' => t(''),
    '#description' =>  t(""),
      '#size' => 40,
    '#required' => FALSE,  
    '#default_value' => $server,
    '#prefix' => '<td>'. ($server ? l('http://'.$server, 'http://'.$server, array('attributes' => array('target' => '_blank'))) : ''),
    '#suffix' => '</td>',
   ); 
   
  $form['remote_servers']["blink_server_weight_{$num}"] = array(
      '#type' => 'textfield',
      '#title' => t(''),
      '#description' =>  t(""),
      '#required' => FALSE,
      '#default_value' => $weight,
      '#size' => 3,
      '#prefix' => '<td>',
      '#suffix' => '</td>',
  );

  if (is_array($server_fields)) {
    $form['remote_servers']["blink_server_remove_{$num}"] = array(
        '#type' => 'checkbox',
        '#title' => t(''),
        '#description' =>  t(""),
        '#required' => FALSE,
        '#default_value' => FALSE,
        '#prefix' => '<td>',
        '#suffix' => '</td>',
    );
  } else $form['remote_servers']["blink_server_remove_{$num}"] = array('#value' => '<td></td>',);
  
  $form['remote_servers']["blink_server_blsid_{$num}"] = array(
      '#type' => 'hidden',
      '#value' => $blsid, 
  );  
  
  $form['remote_servers']["markup_field_servers_row_{$num}_end"] = array(
    '#value' => '</tr>',
  );

}

function blink_admin_settings_validate($form, &$form_state) {
  // check if the user clicked "reset to defaults" button and flag to reload defaults when displaying form next
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  if ($op == t('Reset to defaults')) return;   
  $server_list = array();
  for ($i=0; $i<20; $i++) if ($server = $form_state['values']["blink_server_{$i}"]) {
    $removed = $form_state['values']["blink_server_removed_{$i}"];
    $blsid = $form_state['values']["blink_server_blsid_{$i}"];
    if (!$removed) {
     if (in_array($server, $server_list)) { // duplicate server name
       form_set_error('', t('Each server in the list must be unique.'));
     } else {
       $server_list[] = $server;
       if (!$form_state['values']["blink_server_weight_{$i}"]) $form_state['values']["blink_server_weight_{$i}"] = 1;
       if (strpos($server, '://')) $form_state['values']["blink_server_{$i}"] = trim(substr($server, strpos($server, '://')+3));
     }
    }
  }
}

function blink_admin_settings_submit($form, &$form_state) {
  // check if the user clicked "reset to defaults" button and flag to reload defaults when displaying form next
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  if ($op == t('Reset to defaults')) {
    db_query('DELETE FROM {blink_servers}');
    variable_set('blink_defaults_reset', TRUE);
    blink_clear_all_server_form_fields($form_state);
    return;
  }

  // rebuild css file
  $class_name = $form_state['values']["blink_link_class"];
  $class_style = $form_state['values']["blink_link_style"];
  $class_hover = $form_state['values']["blink_link_hover"];
  blink_rebuild_css($class_name, $class_style, $class_hover);
  

  // SERVERS
  // update weight of local server if changed
  if ($local_weight = (int) $form_state['values']["blink_local_links_weight"]) {
    $old_weight = db_result(db_query('SELECT server_weight FROM {blink_servers} WHERE server="local"'));
    if ($local_weight != $old_weight) {
      drupal_set_message("Updated local goals weight");
      db_query('UPDATE {blink_servers} SET server_weight=%d WHERE server="local"', $local_weight);
    }
    unset($form_state['values']["blink_local_links_weight"]);
  }    
  // remove deleted servers
  for ($i=0; $i<20; $i++) {
    $server_name = $form_state['values']["blink_server_{$i}"]; 
    $blsid = $form_state['values']["blink_server_blsid_{$i}"];
    $remove_checked = $form_state['values']["blink_server_remove_{$i}"]; 
    if ($remove_checked && $blsid) {
      $server_name = $server_name ? $server_name : db_result(db_query("SELECT server FROM {blink_servers} WHERE blsid=%d", $blsid));
      drupal_set_message("Removed Blink server '$server_name'");
      db_query('DELETE FROM {blink_links} WHERE gid IN (SELECT gid FROM {blink_goals} WHERE blsid=%d)', $blsid);
      db_query('DELETE FROM {blink_goals} WHERE blsid=%d', $blsid); // drupal_set_message("Query flopped");
      db_query('DELETE FROM {blink_servers} WHERE blsid=%d', $blsid);
      blink_clear_server_form_field($form_state, $i);
    }    
  }   
  // update remaining server rows in table
  $rec = new stdClass();
  for ($i=0; $i<20; $i++) if ($rec->server = $form_state['values']["blink_server_{$i}"]) {
    $rec->server_weight = (int) $form_state['values']["blink_server_weight_{$i}"] ? (int) $form_state['values']["blink_server_weight_{$i}"] : 1;
    $rec->blsid = $form_state['values']["blink_server_blsid_{$i}"] ? $form_state['values']["blink_server_blsid_{$i}"] : NULL;
    $rec->server_key = $rec->server_key ? $rec->server_key : uniqid('b', TRUE);
    $blsid_key = $rec->blsid ? array('blsid') : NULL;
    drupal_write_record('blink_servers', $rec, $blsid_key);
    blink_clear_server_form_field($form_state, $i); // no point saving these
  }
  
  blink_clear_all_server_form_fields($form_state); // just to be sure these are not saved by system settings submit

  // LOCAL GOALS
  // remove, add or update goals
  $local_blsid = db_result(db_query('SELECT blsid FROM {blink_servers} WHERE server="local"'));
  $max_goals = db_result(db_query("SELECT COUNT(*)+1 FROM {blink_goals} WHERE blsid=%d", $local_blsid)); 
  for ($i=0; $i<$max_goals; $i++) {
    // of course we need both kw and url
    //$goal['kw'] = $form_state['values']["blink_local_goal_kw_{$i}"];
    $goal['kw'] = trim($form_state['values']["blink_local_goal_kw_{$i}"]);
    $goal['url'] = trim(trim($form_state['values']["blink_local_goal_url_{$i}"]), '/');

    if ($goal['url'] && $goal['kw']) {
      // try to find a matching old record to see if this is just an update
      if ($old_gid = $form_state['values']["blink_local_goal_gid_{$i}"]) {
        $old = db_fetch_array(db_query('SELECT * FROM {blink_goals} WHERE gid=%d', $old_gid)); 
      } else $old = db_fetch_array(db_query('SELECT * FROM {blink_goals} WHERE blsid=%d AND kw="%s" AND url="%s"', $local_blsid, $goal['kw'], $goal['url']));
      
      // drupal_set_message("Old_gid: {$old_gid} from local server matching record: <pre>". print_r($old, TRUE) ."</pre>"); continue; 

      // is this a delete?
      $remove_checked = $form_state['values']["blink_local_goal_remove_{$i}"];
      if ($remove_checked && $old['gid']) {
        drupal_set_message("Removed local goal '{$old['kw']}'");
        db_query('DELETE FROM {blink_links} WHERE gid=%d', $old['gid']);
        db_query('DELETE FROM {blink_goals} WHERE gid=%d', $old['gid']);
        blink_clear_local_goal_form_field($form_state, $i);
        continue;
      }
      // otherwise, it's an insertion or an update

      // if it exaclty matches an existing record, just continue on
      $weight = (int) $form_state['values']["blink_local_goal_weight_{$i}"];
       $goal['weight'] = $weight ? $weight : 1; 
      if (($old['weight'] == $goal['weight']) && ($old['url'] == $goal['url']) && ($old['kw'] == $goal['kw'])) { 
        blink_clear_local_goal_form_field($form_state, $i);
        continue;        
      }
      
      // finally, add or update the modified record
      $goal['gid'] = $old ? $old['gid'] : 0; // if no gid, use old gid if exists 
      $goal['url'] = $old ? $goal['url'] : url($goal['url'], array('absolute')); 
       $goal['url'] = trim($goal['url'], '/');
      $goal['blsid'] = $local_blsid;
      $goal['goal_updated'] = time();
      $goal['goal_source'] = 'blink'; 
      $goal['gid'] = $old ? $old['gid'] : 0;
      $goal['goal_uid'] = $old ? $old['goal_uid'] : uniqid('G', TRUE);
      $gid_key = $goal['gid'] ? array('gid') : NULL;
      // if ($remove_checked) drupal_set_message("Removing local goal: <pre>". print_r($goal, TRUE) . "</pre>");  
      if ($old) drupal_set_message("Updated local goal weight: {$goal['kw']} -> {$goal['weight']}");
        else drupal_set_message("Added new local goal: {$goal['kw']} -> {$goal['url']}");
      drupal_write_record('blink_goals', $goal, $gid_key); 
    }
    blink_clear_local_goal_form_field($form_state, $i);
  }
 

  // update local weights of every single goal since a change to any goal weight requires this
  include_once(drupal_get_path('module', 'blink') .'/blink_update.inc.php');
  blink_update_all_goal_localweights();
  
  // Node de-selection: clean up deselected node values
  $types = $form_state['values']['blink_exclude_types'];
  foreach ($types as $key=>$type) if (!$type) unset($types[$key]);
  variable_set('blink_exclude_types', array_keys($types));
  unset($form_state['values']['blink_exclude_types']);

  // some important requirements not set on the settings page
  // site email and host
  if (!valid_email_address(variable_get('site_mail', ''))) drupal_set_message(t('To participate you must have a valid E-mail address: ') .
    l('configure site', "/admin/settings/site-information"), 'warning');
   
  // make sure the user's host is valid
  global $base_url;
  $parts = parse_url($base_url);
  $this_server = $parts['host'];
  if (!valid_url($this_server)) drupal_set_message(t('To participate you must have a valid \$base_url: '), 'warning');

  // settings changes will result in invalidating page caches
  cache_clear_all(NULL, 'cache_page');
  
}

function blink_clear_all_server_form_fields(&$form_state) {
 for ($i=0; $i<20; $i++) blink_clear_server_form_field($form_state, $i, TRUE);
}

function blink_clear_server_form_field(&$form_state, $num, $unset=FALSE) { 
 $form_state['values']["blink_server_{$num}"] ='';
 $form_state['values']["blink_server_weight_{$num}"] = '';
 $form_state['values']["blink_server_remove_{$num}"] = '';
 $form_state['values']["blink_server_blsid_{$num}"] = '';
 if ($unset) {
   unset($form_state['values']["blink_server_{$num}"]);
   unset($form_state['values']["blink_server_weight_{$num}"]);
   unset($form_state['values']["blink_server_remove_{$num}"]);
   unset($form_state['values']["blink_server_blsid_{$num}"]);
 } 
}

function blink_clear_local_goal_form_field(&$form_state, $num, $unset=FALSE) { 
 $form_state['values']["blink_server_{$num}"] ='';
 $form_state['values']["blink_server_weight_{$num}"] = '';
 $form_state['values']["blink_server_remove_{$num}"] = '';
 $form_state['values']["blink_server_blsid_{$num}"] = '';
 if ($unset) {
   unset($form_state['values']["blink_server_{$num}"]);
   unset($form_state['values']["blink_server_weight_{$num}"]);
   unset($form_state['values']["blink_server_remove_{$num}"]);
   unset($form_state['values']["blink_server_blsid_{$num}"]);
 } 
}

function blink_sort_by_weight($a, $b) {
 return $a['weight'] < $b['weight'];
}

function blink_sort_by_server_weight($a, $b) {
 return $a['server_weight'] < $b['server_weight'];
}

function blink_get_default_servers() {
  // add default servers to list
  $file = drupal_get_path('module', 'blink') .'/blink_defaults.ini.php';
  if (!file_exists($file)) return;
  $ini = parse_ini_file($file, TRUE);
  foreach ($ini['servers'] as $server=>$weight) {
    if (!$server) continue;
    if (!$weight) $weight = 1;
    $rec->server = $server;
    $rec->server_weight = $weight;
    $exists = db_result(db_query('SELECT blsid FROM {blink_servers} WHERE server="%s"', $rec->server));
    if (!$exists) {
     drupal_write_record('blink_servers', $rec);
     drupal_set_message('Added Blink server: '. $server);
    }
  }
  variable_set('blink_defaults_reset', FALSE);
} 

function blink_server_connected($server_url) {
  if (empty($server_url)) return FALSE;
  $response = xmlrpc($server_url, 'system.listMethods');
  // drupal_set_message("Testing RPC URL '{$server_url}', (system.listMethods):  <pre>". print_r($response, true). "</pre>");
  if (is_array($response)) {
    return in_array('linklistMaintainer.getLinklist', $response);
  }
}

// cached - check this url and append xmlrpc.php if necessary, then check again and store resulting working url
function blink_get_server_RPC_url($server_name) {
 $server_name = rtrim($server_name, '/');
 if ($url = variable_get('blink_server_url_'.$server_name, '')) return $url;
 $server = db_fetch_array(db_query('SELECT * from {blink_servers} WHERE server="%s"', $server_name)); 
 // drupal_set_message("<pre>". print_r($server, true). "</pre>");
 $url = 'http://'. trim($server_name);
 if (blink_server_connected($url)) {
   variable_set('blink_server_url_'.$server_name, $url);
   return $url;
 } else {
   $url = $url .'/xmlrpc.php';
   if (blink_server_connected($url)) {
     variable_set('blink_server_url_'.$server_name, $url);
     return $url;
   }
 }
}

function blink_rebuild_css($class, $style, $hover) {
  if (!$class) return;
  $file = file_directory_path() .'/'. check_plain($class) .'.css';
  $data = " \n    a.{$class} {{$style}} \n    a.{$class}:hover {{$hover}} \n ";
  file_save_data($data, $file, FILE_EXISTS_REPLACE);
  variable_set('blink_link_css_filepath', $file);
}

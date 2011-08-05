<?php
// $Id$

/**
 * @file
 * Blink client linking module
 *
 * Administrative forms and report pages
 */
 
if (!function_exists('db_fetch_array')) {
  function db_fetch_array($query) {
    if (is_object($query)) return $query->fetchAssoc();
  }
}
if (!function_exists('db_value')) {
  function db_value($query) {
    if (is_object($query)) return $query->fetchField();
  }
}

 
      
function blink_link_report() {
  drupal_add_css(drupal_get_path('module', 'blink') . '/blink_form.css', 'module', 'all', FALSE);
  drupal_add_js(drupal_get_path('module', 'blink') . '/blink_form.js'); 
  
  //return "hello world"; return;
  
  // local links
    // Local link goals by weight (12 goals, 365 links on 87 pages)
      // kw, url, weight, # links, # pages
      // ..  
  // gather up all goals for this server
  // $local_blsid = db_result(db_query('SELECT blsid FROM {blink_servers} WHERE server="local"'));
  $local_blsid = db_query("SELECT blsid FROM {blink_servers} WHERE server='local'")->fetchField();   
  $ret = db_query("SELECT g.*, COUNT(*) link_count FROM {blink_goals} g, {blink_links} l  
     WHERE g.gid=l.gid AND g.blsid=:local_blsid GROUP by g.gid ORDER BY g.weight DESC", 
      array(':local_blsid' => $local_blsid)); 
    
    
         
    
  while ($goal = db_fetch_array($ret)) $goals[] = $goal;  
  if (count($goals)) {
    // for each site
    $link_count = 0; foreach ($goals as $goal) $link_count += $goal['link_count']; 
    $page_count = db_query("SELECT COUNT(DISTINCT l.nid) page_count FROM {blink_goals} g, {blink_links} l  
       WHERE g.gid=l.gid AND g.blsid=:local_blsid",  array(':local_blsid' => $local_blsid));
    $goals_list = "<ul class='goals'> \n"; 
 
    // for each goal in this site
    foreach ($goals as $goal) {
      // kw, url, weight, # links, # pages
      $goals_list .= "  <li> <u>{$goal['kw']}</u> -> {$goal['url']}: {$goal['link_count']} links. </li> \n";
    }
    $local_report .= $goals_list . '</ul></li>';
    // main header for local server links
    $totals = '(' . count($goals) . " goals met with {$link_count} links on {$page_count} pages.)";
    $local_report = "<h2 class='local_server'> Local Links: {$totals} </h2> <ol class='goal_list'>" . $local_report . '</ol>'; 
  }
   
  
  // remote links: (89 goals, 1,250 links on 240 page)
    // example.com goals: (site_weight, 25 goals, 450 links on 92 pages)
      // kw, url, weight, # links
      // .. 
    // example2.com goals: (site_weight, 25 goals, 450 links on 92 pages)
      // kw, url, weight, # links
      // ..
  $ret = db_query("SELECT blsid,server,server_weight FROM {blink_servers} WHERE blsid<>:local_blsid ORDER BY server_weight DESC",   
                  array(':local_blsid' => $local_blsid));
                  
  //    $ret = db_query("SELECT blsid,server,server_weight FROM {blink_servers} ORDER BY server_weight DESC");
  while ($server = db_fetch_array($ret)) $servers[] = $server;
  if (count($servers)) {
    foreach ($servers as $server) {
      // gather up all goals for this server
      $goals = array();
      $ret = db_query("SELECT g.*, COUNT(*) link_count FROM {blink_goals} g, {blink_links} l  
         WHERE g.gid=l.gid AND g.blsid=:blsid GROUP by g.gid ORDER BY g.weight DESC", array(':blsid' => $server['blsid']));
        
      while ($goal = db_fetch_array($ret)) $goals[] = $goal;
      if (count($goals)) {
        // for each site
        $link_count = 0; foreach ($goals as $goal) $link_count += $goal['link_count'];
        $page_count = db_query("SELECT COUNT(DISTINCT l.nid) page_count FROM {blink_goals} g, {blink_links} l " .
            "WHERE g.gid=l.gid AND g.blsid=:blsid", array(':blsid' => $server['blsid']))->fetchField();
            
        // example.com goals: (site_weight, 25 goals, 450 links on 92 pages)
        $servers_report .= "<li><h3 class='remote_server'> <b>" . l($server['server']) . "</b> goals: (" . count($goals) . 
          " goals, " . "{$link_count} links on {$page_count} pages) </h3>";
        $goals_list = "<ul class='goals'> \n";
        // for each goal in this site
        foreach ($goals as $goal) {
          // kw, url, weight, # links, # pages
          $goals_list .= "  <li> <u>{$goal['kw']}</u> -> {$goal['url']}: {$goal['link_count']} links. </li> \n";
        }
        $servers_report .= $goals_list . '</ul></li>';
        $total_links += $link_count;
        $total_goals += count($goals);
        $total_pages += $page_count;
      }
    }
    // main header for remote server links
    if (count($servers)>0) $totals = '(' . count($servers) . " servers with {$total_goals} goals met, {$total_links} links on {$total_pages} pages.)";
    $servers_report = "<h2 class='remote_servers'> Remote Links: {$totals} </h2> <ol class='goal_list'>" . $servers_report . '</ol>';
  }  
      
  $result = $local_report . $servers_report;
  if (empty($result)) $result = "<h2> No link or server data to report. </h2>";
  return $result;
}

function blink_force_update_now() {
 /*  // for local testing, override server_key with key matching linklist variable
  $server['server_key'] = variable_get('linklist_maintainer_test_key', user_password(40));
  variable_set('linklist_maintainer_test_key', $server['server_key']);
  drupal_set_message('blink_update.inc line 33-36: Set server_key = varible "linklist_maintainer_test_key". <br> Make sure to remove this code after testing', 'warning');
 */
  
  // update goals and links whenever the admin screen is viewed -- for development
  //blink_update_all_servers();
  //drupal_goto('admin/settings/blink/settings');   
}

function blink_admin_settings_form() {  
  drupal_add_css(drupal_get_path('module', 'blink') . '/blink_form.css', 'module', 'all', FALSE);
  drupal_add_js(drupal_get_path('module', 'blink') . '/blink_form.js');  
  $stopwatch = new StopWatch();   
  
  // list of local links 
  blink_form_local_links($form); //drupal_set_message('blink_form_local_links: ' . $stopwatch->elapsed());

  // list of remote servers plus one extra field row to add a server 
  blink_form_remote_servers($form);  //drupal_set_message('blink_form_remote_servers: ' . $stopwatch->elapsed());

  // Site Settings 
   blink_form_site_settings($form); //drupal_set_message('blink_form_site_settings: ' . $stopwatch->elapsed());

  // page selections 
  blink_form_page_selections($form);  //drupal_set_message('blink_form_page_selections: ' . $stopwatch->elapsed());

  //$form['#validate'][] = 'blink_admin_settings_validate';
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
  $table_header = '<table class="blink_servers">' .
    '<tr> <th></th> <th>' . t('Server') . '</th><th>' . t('Weight') . '</th><th>' . t('Remove') . '</th> </tr>';
  $form['remote_servers']['markup_servers_table'] = array('#markup' => $table_header);
  // weight of local links
  $local_server = db_query("SELECT * FROM {blink_servers} WHERE server='local'")->fetchAssoc(); 
  $form['remote_servers']['blink_local_links_weight'] = array(
      '#type' => 'textfield',
      '#required' => TRUE,
      '#default_value' => $local_server['server_weight'],
      '#size' => 3,
      '#prefix' => '<tr><td></td><td>' . t('Local link goals') . '</td><td>',
      '#suffix' => '</td></td><td></tr>',
  );
  // re-gather the defaults if the form has been reset
  if (variable_get('blink_defaults_reset', FALSE)) blink_get_default_servers();
    // list of remote servers, ordered by weight
  if ($ret = db_query("SELECT * FROM {blink_servers} WHERE server<>'local'")) {
    while ($server = db_fetch_array($ret)) $servers[] = $server; 
    if ($servers) {
      usort($servers, 'blink_sort_by_server_weight'); 
      foreach ($servers as $key => $server) blink_server_form_add_server_field($form, $key, $server);
      // an extra, empty row to add another server
      blink_server_form_add_server_field($form, count($servers));
    }
  }
  // close html table
  $form['remote_servers']['markup_servers_table_end'] = array('#markup' => '</table>');
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
    '#options' => array(1 => 1, 2 => 2, 3 => 3, 4 => 4, 5 => 5, 6 => 6, 7 => 7, 8 => 8, 9 => 9, 10 => 10, 20 => 20),
    '#description' => t('Maximum number of links to be added to any given page'),
  );
  

  $form['site_settings']['link_class_settings']  = array(
    '#type'         => 'fieldset',
    '#title'        => t('Link Markup'),
    '#collapsible'  => TRUE,
    '#collapsed'    => TRUE,
  ); 

  // class to use for links
  $form['site_settings']['link_class_settings']['blink_link_class'] = array(
    '#type' => 'textfield',
    '#title' => t('Blink link class'),
    '#default_value' => variable_get('blink_link_class', ''),
    '#description' => t('Optional CSS class to assign to links generated by Blink'),
  );

  $form['site_settings']['link_class_settings']['blink_link_style'] = array(
    '#type' => 'textfield',
    '#title' => t('Blink link style'),
    '#default_value' => variable_get('blink_link_style', 'text-decoration:none; border-bottom:1px dashed silver;'),
    '#description' => t('Optional CSS style to be applied to links'),
  );

  $form['site_settings']['link_class_settings']['blink_link_style_hover'] = array(
    '#type' => 'textfield',
    '#title' => t('Blink link hover style'),
    '#default_value' => variable_get('blink_link_style_hover', 'background:#FF9; border-bottom:1px solid;'),
    '#description' => t('Optional CSS style to be applied to links on hover'),
  );
/**/
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
  $types = array_keys(node_type_get_types());
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
    '#description'  => t('Links will not be created for paths entered here. Enter one path per line. <br />' . 
         "'*' is a wildcard character for any character except '/'"),
  );
}
 
function blink_form_local_links(&$form) {
  // list of local links plus an extra to add a new link
  // ***************************************************************
  // list of local goals, ordered by weight
  $goals = array();
  $local_blsid = db_query("SELECT blsid FROM {blink_servers} WHERE server='local'")->fetchField();
  $ret = db_query("SELECT * FROM {blink_goals} WHERE blsid=:local_blsid ORDER BY weight DESC", 
    array(':local_blsid' => $local_blsid));
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
   
  foreach ($goals as $key => $goal) $goals[$key]['num'] = $key; // store the num with each 
  foreach ($goals as $goal) $goal_groups[$goal['goal_source']][] = $goal;
  //drupal_set_message('Goals: <pre>'. print_r($goal_groups, TRUE) . "</pre>");
  //if (!$goal_groups) return;
  
  // first, display Blink local links plus an add row (since blink links can be added manually)
  if ($goal_groups) foreach ($goal_groups as $group_name => $group) if ($group_name=='blink') {
     blink_local_form_add_goal_group($form, $group, 'blink', $total_count);
     $blink_links_found = TRUE;
     break;
  }
  // if no blink links found, display the section with 'add' row anyway
  if (!$blink_links_found) blink_local_form_add_goal_group($form, '', 'blink', $total_count); 
  
  // now display any other groups of links
  if ($goal_groups) foreach ($goal_groups as $group_name => $group) if ($group_name!='blink') blink_local_form_add_goal_group($form, $group, $group_name, $total_count);   

}

function blink_local_form_add_goal_group(&$form, $group, $group_name, $total_count=0) {  
  if (!$group_name) return;
  
  // Local Links 
  $form['local_goals'][$group_name]  = array(
    '#type'         => 'fieldset',
    '#title'        => $group_name,
    '#collapsible'  => TRUE,
    '#collapsed'    => ($group_name!='blink'),  
  ); 
   
  if (!count($group)) {
    $form['local_goals'][$group_name]['markup_local_goals_table'] = array('#markup' => t('No Keywords from this goal source'));
    return; 
  }

  
  // variable_set("kwgoals_blink_readonly", TRUE);  
  $read_only = variable_get("{$group_name}_blink_readonly", FALSE);
  if ($read_only) { 
    foreach ($group as $goal) { 
      $rows[] = array( 
        $goal['kw'], 
        l($goal['url'], $goal['url'], array('attributes' => array('target' => '_blank'))),
        $goal['weight'] ? $goal['weight'] : 1
      ); 
    }  
    $table_data = array (
      'header' => array(t('Keyword or Phrase'), t('Target URL'), t('Weight')), 
      'rows' => $rows,
      'empty' => t('No keywords for this group exist yet'),
      'attributes' => array(),
      'caption' => NULL,
      'colgroups' => NULL,
       'sticky' => NULL, 
    );
    $table = theme_table($table_data);
   // $table.= "<pre>". print_r($rows, true) ."</pre>";
     
    $form['local_goals'][$group_name]['markup_local_goals_table'] = array('#markup' => $table); 
  }
  else {
    // servers table
    $table_header = '<table class="blink_local_goals">' . '<tr> <th></th> <th>' . t('Keyword or Phrase') . '</th> <th>' . t('Target URL') . '</th> <th>' . t('Weight') . '</th><th>' . t('Remove') . '</th> </tr>'; 
    $form['local_goals'][$group_name]['markup_local_goals_table'] = array('#markup' => $table_header); 
  
    // fetch one field for each exiting local link 
    if (is_array($group) && count($group)) foreach ($group as $goal) {
      blink_local_form_add_goal_field($form, $goal['num'], $goal, $group_name);  
    }
    // an extra, empty row to add another server   
    if ($group_name == 'blink') $options[] = blink_local_form_add_goal_field($form, $total_count, '', $group_name); 
  
    // close html table
    $form['local_goals'][$group_name]['markup_servers_table_end'] = array('#markup' => '</table>');
  }
 
}

function blink_local_form_add_goal_field(&$form, $num=0, $goal='', $group_name='blink') {
  $exists = is_array($goal); 
  $weight = $goal['weight'] ? $goal['weight'] : 1;
  $blsid  = $goal['blsid']; 
  if (!$exists) $connect_image = '<img src="/' . drupal_get_path('module', 'blink') . '/add.png' . '" title="Add new server" width="16" />'; 
 
  $form['local_goals'][$group_name]["markup_field_servers_row_{$num}"] = array(
    '#markup' => "<tr>",
  );
  $form['local_goals'][$group_name]["markup_field_servers_row_header_{$num}"] = array(
     '#markup' => '<td>' . $connect_image . '</td>',
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
    '#prefix' => '<td>' . $goal['kw'],
    '#suffix' => '</td>',
  );
  $form['local_goals'][$group_name]["blink_local_goal_url_{$num}"] = array(
    '#type' =>  $exists ? 'hidden' : 'textfield',
    '#title' => t(''),
    '#description' =>  t(''),
    '#required' => FALSE,
    '#default_value' => $goal['url'],
    '#size' => 35,
    '#prefix' => '<td>' . l($goal['url'], $goal['url'], array('attributes' => array('target' => '_blank'))),
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
   } 
   else $form['local_goals'][$group_name]["blink_local_goal_remove_{$num}"] = array('#markup' => '<td></td>');
      
  
  $form['local_goals'][$group_name]["markup_field_servers_row_{$num}_end"] = array(
    '#markup' => '</tr>',
  );

} 

function blink_server_form_add_server_field(&$form, $num, $server_fields='') {
  if (is_array($server_fields)) {
    $weight = $server_fields['server_weight'];
    $server = $server_fields['server'];
    $blsid  = $server_fields['blsid'];
    
    $can_connect = blink_get_server_RPC_url($server);

    if ($can_connect) $connect_image = '<img src="/' . drupal_get_path('module', 'blink') . '/checked.png' . '" title="Connection OK" width="15" />';
     else $connect_image = '<img src="/' . drupal_get_path('module', 'blink') . '/failed.png' . '" title="Connection Failed!" width="15" />';
  } 
  else {
    $connect_image = '<img src="/' . drupal_get_path('module', 'blink') . '/add.png' . '" title="Add new server" width="16" />';
  }
 
  $form['remote_servers']["markup_field_servers_row_{$num}"] = array(
    '#markup' => "<tr>",
  );
  $form['remote_servers']["markup_field_servers_row_header_{$num}"] = array(
     '#markup' => '<td>' . $connect_image . '</td>',
  );

  $form['remote_servers']["blink_server_{$num}"] = array(
    '#type' => $server ? 'hidden' : 'textfield',
    '#title' => t(''),
    '#description' =>  t(""),
      '#size' => 40,
    '#required' => FALSE,  
    '#default_value' => $server,
    '#prefix' => '<td>' . ($server ? l($server, 'http://' . $server, array('attributes' => array('target' => '_blank'))) : ''),
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
  } 
  else $form['remote_servers']["blink_server_remove_{$num}"] = array('#markup' => '<td></td>');
  
  $form['remote_servers']["blink_server_blsid_{$num}"] = array(
      '#type' => 'hidden',
      '#value' => $blsid, 
  );  
  
  $form['remote_servers']["markup_field_servers_row_{$num}_end"] = array(
    '#markup' => '</tr>',
  );

}

function blink_admin_settings_validate($form, &$form_state) {
  //$stopwatch = new StopWatch();  
  // check if the user clicked "reset to defaults" button and flag to reload defaults when displaying form next
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  if ($op == t('Reset to defaults')) return;   
  $server_list = array();
  for ($i=0; $i<20; $i++) if (isset($form_state['values']["blink_server_{$i}"])) {
    $server = $form_state['values']["blink_server_{$i}"];
    $removed = $form_state['values']["blink_server_removed_{$i}"];
    $blsid = $form_state['values']["blink_server_blsid_{$i}"];
    if (!$removed) {
     if (in_array($server, $server_list)) { // duplicate server name
       form_set_error('', t('Each server in the list must be unique.'));
     } 
     else {
       $server_list[] = $server;
       if (!$form_state['values']["blink_server_weight_{$i}"]) $form_state['values']["blink_server_weight_{$i}"] = 1;
       if (strpos($server, '://')) $form_state['values']["blink_server_{$i}"] = trim(substr($server, strpos($server, '://')+3));
     }
    }
  }
  //drupal_set_message('blink_admin_settings_validate: ' . $stopwatch->elapsed());
}

function blink_admin_settings_submit($form, &$form_state) {
 $stopwatch = new StopWatch();   

   
  // check if the user clicked "reset to defaults" button and flag to reload defaults when displaying form next
  $op = isset($form_state['values']['op']) ? $form_state['values']['op'] : '';
  if ($op == t('Reset to defaults')) {
    // db_query('DELETE FROM {blink_servers}');
    db_delete('blink_servers')->execute();
    variable_set('blink_defaults_reset', TRUE);
    blink_clear_all_server_form_fields($form_state);
    return;
  }

  // rebuild css file
  $class_name = isset($form_state['values']["blink_link_class"]) ? $form_state['values']["blink_link_class"] : '';
  $class_style = isset($form_state['values']["blink_link_style"]) ? $form_state['values']["blink_link_style"] : '';
  $class_hover = isset($form_state['values']["blink_link_hover"]) ? $form_state['values']["blink_link_hover"] : '';
  if ($class_name) blink_rebuild_css($class_name, $class_style, $class_hover); 

  // SERVERS
  // update weight of local server if changed
  if (isset($form_state['values']["blink_local_links_weight"])) {
    $local_weight = (int) $form_state['values']["blink_local_links_weight"];
    $old_weight = db_query("SELECT server_weight FROM {blink_servers} WHERE server='local'")->fetchField();
    if ($local_weight != $old_weight) {
      drupal_set_message(t("Updated local goals weight")); 
      db_update('blink_servers')->fields(array('server_weight', $local_weight))->condition('server', 'local')->execute();
    }
    unset($form_state['values']["blink_local_links_weight"]);
  }    
   
//drupal_set_message('blink_admin_settings_submit: ' . $stopwatch->elapsed()); $stopwatch->reset(); 

  // remove deleted servers
  for ($i=0; $i<20; $i++) {
   $server_name = isset($form_state['values']["blink_server_{$i}"]) ? $form_state['values']["blink_server_{$i}"] : ''; 
   $blsid = isset($form_state['values']["blink_server_blsid_{$i}"]) ? $form_state['values']["blink_server_blsid_{$i}"] : '';
   $remove_checked = isset($form_state['values']["blink_server_remove_{$i}"]) ? $form_state['values']["blink_server_remove_{$i}"] : ''; 
   if ($remove_checked && $blsid) {
//drupal_set_message("For some reason this was called.");
      $server_name = $server_name ? $server_name : db_result(db_query("SELECT server FROM {blink_servers} WHERE blsid=:blsid", array(':blsid' => $blsid)));
      drupal_set_message(t("Removed Blink server '@name'", array('@name' => $server_name)));
      // db_query('DELETE FROM {blink_links} WHERE gid IN (SELECT gid FROM {blink_goals} WHERE blsid=:blsid)', array(':blsid' => $blsid));
      $query = db_select('blink_goals')->fields(array('gid'))->condition('blsid', $blsid);
      db_delete('blink_links')->condition('gid', $query, 'IN')->execute();  
      // db_query('DELETE FROM {blink_goals} WHERE blsid=%d', $blsid); // drupal_set_message("Query flopped");
      db_delete('blink_goals')->condition('blsid', $blsid)->execute();  
      // db_query('DELETE FROM {blink_servers} WHERE blsid=%d', $blsid);
      db_delete('blink_servers')->condition('blsid', $blsid)->execute(); 
      
      blink_clear_server_form_field($form_state, $i);
    } 
  }  

//drupal_set_message('blink_admin_settings_submit 0: ' . $stopwatch->elapsed()); $stopwatch->reset();
 
  // update remaining server rows in table
  $rec = new stdClass();
  for ($i=0; $i<20; $i++) if (isset($form_state['values']["blink_server_{$i}"])) {
    $rec->server = $form_state['values']["blink_server_{$i}"];
    $rec->server_weight = (int) $form_state['values']["blink_server_weight_{$i}"] ? (int) $form_state['values']["blink_server_weight_{$i}"] : 1;
    $rec->blsid = $form_state['values']["blink_server_blsid_{$i}"] ? $form_state['values']["blink_server_blsid_{$i}"] : NULL;
    $rec->server_key = $rec->server_key ? $rec->server_key : uniqid('b', TRUE);
    $blsid_key = $rec->blsid ? array('blsid') : NULL;
    drupal_write_record('blink_servers', $rec, $blsid_key);
    blink_clear_server_form_field($form_state, $i); // no point saving these
  } 
  blink_clear_all_server_form_fields($form_state); // just to be sure these are not saved by system settings submit
  
//drupal_set_message('blink_admin_settings_submit 1: ' . $stopwatch->elapsed());

  // LOCAL GOALS
  // remove, add or update goals
  $local_blsid = db_query("SELECT blsid FROM {blink_servers} WHERE server='local'")->fetchField();
  $max_goals = db_query("SELECT COUNT(*)+1 FROM {blink_goals} WHERE blsid=:local_blsid", array(':local_blsid' => $local_blsid))->fetchField(); 
  
//drupal_set_message('blink_admin_settings_submit 2: ' . $stopwatch->elapsed()); $stopwatch->reset(); 
 
 
  for ($i=0; $i<$max_goals; $i++) {
    // of course we need both kw and url
    //$goal['kw'] = $form_state['values']["blink_local_goal_kw_{$i}"];
    $goal['kw'] = isset($form_state['values']["blink_local_goal_kw_{$i}"]) ? trim($form_state['values']["blink_local_goal_kw_{$i}"]) : '';
    $goal['url'] = isset($form_state['values']["blink_local_goal_url_{$i}"]) ? trim(trim($form_state['values']["blink_local_goal_url_{$i}"]), '/') : '';

    if ($goal['url'] && $goal['kw']) {
      // try to find a matching old record to see if this is just an update
      if ($old_gid = $form_state['values']["blink_local_goal_gid_{$i}"]) {
        $old = db_query("SELECT * FROM {blink_goals} WHERE gid=:old_gid", array(':old_gid' => $old_gid))->fetchAssoc(); 
      } 
      else $old = db_query("SELECT * FROM {blink_goals} WHERE blsid=:local_blsid AND kw=:kw AND url=:url", 
         array(':local_blsid' => $local_blsid, ':kw' => $goal['kw'], ':url' => $goal['url']))->fetchAssoc();
      
      // drupal_set_message("Old_gid: {$old_gid} from local server matching record: <pre>". print_r($old, TRUE) ."</pre>"); continue; 

      // is this a delete?
      $remove_checked = $form_state['values']["blink_local_goal_remove_{$i}"];
      if ($remove_checked && $old['gid']) {
        drupal_set_message(t("Removed local goal '@old'", array('@old' => $old['kw'])));
        //db_query('DELETE FROM {blink_links} WHERE gid=%d', $old['gid']);
        db_delete('blink_links')->condition('gid', $old['gid'])->execute(); 
        
        //db_query('DELETE FROM {blink_goals} WHERE gid=%d', $old['gid']);
        db_delete('blink_goals')->condition('gid', $old['gid'])->execute(); 
        
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
      if ($old) drupal_set_message(t("Updated local goal weight: @kw -> @weight",
         array('@kw' => $goal['kw'], '@weight' => $goal['weight'])));
        else drupal_set_message(t("Added new local goal: @kw -> @url", array('@kw' => $goal['kw'], '@url' => $goal['url'])));
      drupal_write_record('blink_goals', $goal, $gid_key); 
    }
    blink_clear_local_goal_form_field($form_state, $i);
  }
 
//drupal_set_message('blink_admin_settings_submit 3: ' . $stopwatch->elapsed()); $stopwatch->reset();  

  // update local weights of every single goal since a change to any goal weight requires this
  include_once('blink_update.inc');
  blink_update_all_goal_localweights();  
//drupal_set_message('blink_admin_settings_submit 4: ' . $stopwatch->elapsed()); $stopwatch->reset();
  
  
  // Node de-selection: clean up deselected node values 
  if (isset($form_state['values']['blink_exclude_types'])) {
    $types = $form_state['values']['blink_exclude_types']; 
    foreach ($types as $key => $type) if (!$type) unset($types[$key]);
    variable_set('blink_exclude_types', array_keys($types));
    unset($form_state['values']['blink_exclude_types']);
  } 

  // some important requirements not set on the settings page
  // site email and host
  if (!valid_email_address(variable_get('site_mail', ''))) drupal_set_message(t('To participate you must have a valid E-mail address:') . ' ' . 
    l(t('configure site'), "/admin/settings/site-information"), 'warning');
   
  // make sure the user's host is valid
  global $base_url;
  $parts = parse_url($base_url);
  $this_server = $parts['host'];
  if (!valid_url($this_server)) drupal_set_message(t('To participate you must have a valid base_url'), 'warning');

  // settings changes will result in invalidating page caches
  cache_clear_all(NULL, 'cache_page');
drupal_set_message('blink_admin_settings_submit 5: ' . $stopwatch->elapsed());
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
  $file = drupal_get_path('module', 'blink') . '/blink_defaults.ini.php';
  if (!file_exists($file)) return;
  $ini = parse_ini_file($file, TRUE);
  foreach ($ini['servers'] as $server => $weight) {
    if (!$server) continue;
    if (!$weight) $weight = 1;
    $rec->server = $server;
    $rec->server_weight = $weight;
    $exists = db_query("SELECT blsid FROM {blink_servers} WHERE server=:server", 
      array(':server' => $rec->server))->fetchField();
    
    if (!$exists) {
     drupal_write_record('blink_servers', $rec);
     drupal_set_message(t('Added Blink server: @server', array('@server' => $server)));
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
 if ($url = variable_get('blink_server_url_' . $server_name, '')) return $url;
 $server = db_fetch_array(db_query("SELECT * from {blink_servers} WHERE server=:server", array(':server' => $server_name))); 
 // drupal_set_message("<pre>". print_r($server, true). "</pre>");
 $url = 'http://' . trim($server_name);
 if (blink_server_connected($url)) {
   variable_set('blink_server_url_' . $server_name, $url);
   return $url;
 } 
 else {
   $url = $url . '/xmlrpc.php';
   if (blink_server_connected($url)) {
     variable_set('blink_server_url_' . $server_name, $url);
     return $url;
   }
 }
}

function blink_rebuild_css($class, $style, $hover) {
  if (!$class) return;
  $file = file_directory_path() . '/' . check_plain($class) . '.css';
  $public = 'public://' . check_plain($class) . '.css'; 
  $data = " \n    a.{$class} {{$style}} \n    a.{$class}:hover {{$hover}} \n ";
  file_save_data($data, $public, FILE_EXISTS_REPLACE);
  variable_set('blink_link_css_filepath', $file);
}

 
class StopWatch {
    public $total;
    public $time;
   
    public function __construct() {
        $this->total = $this->time = microtime(true);
    }
   
    public function clock() {
        return -$this->time + ($this->time = microtime(true)) .' ms';
    }
   
    public function elapsed() {
        $result = number_format(round((microtime(true)-$this->total)*1000, 2)) .'ms';
        self::reset();
        return $result;        
    }
   
    public function reset() {
        $this->total=$this->time=microtime(true);
    }
} 




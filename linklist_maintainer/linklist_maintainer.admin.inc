<?php
// $Id$

/**
 * @file
 */

/* ADMINISTRATION FORM
 function linklist_maintainer_admin_form() {
 $form['linklist_maintainer_key_generation'] = array(
 '#type' => 'select',
 '#title' => t('Key Generation'),
 '#options' => array(
 'auto' => t('Automatic key generation (open participation)'),
 'req' => t('Public request form (moderated participation)'),
 'admin' => t('Administrator generates all keys (closed participation)')
 ),
 '#default_value' => variable_get('linklist_maintainer_key_generation', 'auto'),
 );
 return system_settings_form($form);
 } /* end administration form */

// LINKLIST FORM
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_linklist_form($form, &$form_state, $showall = FALSE) {
  if ($showall == 'disabled') {
    $where = 'WHERE lmg.weight = 0';
  }
  elseif ($showall != 'all') {
    $where = 'WHERE lmg.weight > 0';
  }
  foreach (array('' => 'Active (default)', 'disabled' => 'Disabled', 'all' => 'All') as $arg => $t) {
    $filterlinks[] = l(t($t), "admin/config/linklist-maintainer/linklist/$arg");
  }
  $form['filterlink']['#value'] = '<p><b>' . t('Filter list:') . '</b><br/>' . implode(' | ', $filterlinks) . '</p>';
  $form['#tableheader'] = array(
    array('data' => t('Link Phrase / Keyword'), 'field' => 'lmg.kw'),
    array('data' => t('Target URL'), 'field' => 'lmg.url', 'sort' => 'asc'),
    array('data' => t('Weight'), 'field' => 'lmg.weight', 'class' => 'weight centered'),
    array('data' => t('Active'), 'field' => 'lmg.ldate', 'class' => 'date centered'),
    array('data' => t('Links'), 'field' => 'linkcount', 'class' => 'centered'),
    array('data' => t('Sites'), 'field' => 'sitecount', 'class' => 'centered'),
    array('data' => ' '),
  );
  $q = pager_query("SELECT lmg.*, COUNT(DISTINCT lms.pid) sitecount, COUNT(lms.lid) linkcount
    FROM {linklist_maintainer_goals} lmg LEFT JOIN {linklist_maintainer_stats} lms ON lmg.lid = lms.lid $where
    GROUP BY lmg.lid" . tablesort_sql($form['#tableheader']), 200, 0, "SELECT COUNT(*) FROM {linklist_maintainer_goals} lmg $where");
  while ($link = db_fetch_array($q)) {
    $form['table'][$link['lid']] = _linklist_maintainer_admin_link_form($link);
  }
  $form['table']['#tree'] = TRUE;
  $form['#theme'] = 'linklist_maintainer_admin_linklist_form';
  if (user_access('administer linklist')) {
    $form['table']['new'] = _linklist_maintainer_admin_link_form();
    $form['table']['new']['url']['#attributes'] = array('title' => t('If you are pointing to a url on another site, you must use an absolute url (with http://).  All relative urls (without http://) will point to a page on your site.'));
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
    );
    if ($form_state['linklist_maintainer']['badurl']) {
      $form['submit']['#value'] = t('I have changed the url - try it again');
      $form['override'] = array(
        '#type' => 'submit',
        '#value' => t('I really want to create a bad link'),
      );
      $form['table']['new']['kw']['#default_value'] = $form_state['linklist_maintainer']['new']['kw'];
      $form['table']['new']['url']['#default_value'] = $form_state['linklist_maintainer']['new']['url'];
      $form['table']['new']['url']['#attributes'] = array('class' => 'error');
      $form['table']['new']['weight']['#default_value'] = $form_state['linklist_maintainer']['new']['weight'];
    }
  }
  $form['pager'] = array('#value' => theme('pager', array('tags' => NULL, 'element' => 0)));
  return $form;
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_linklist_form_submit($form, &$form_state) {
  if ($form_state['values']['table']['new']['kw'] && $form_state['values']['table']['new']['url']) {
    // TODO Please review the conversion of this statement to the D7 database API syntax.
    /* db_query("INSERT INTO {linklist_maintainer_goals} (kw, goal_uid, url, weight, ldate) VALUES ('%s', '%s', '%s', %d, %d)", $form_state['values']['table']['new']['kw'], uniqid('G', TRUE), $form_state['values']['table']['new']['url'], $form_state['values']['table']['new']['weight'], REQUEST_TIME) */
    $id = db_insert('linklist_maintainer_goals')
  ->fields(array(
      'kw' => $form_state['values']['table']['new']['kw'],
      'goal_uid' => uniqid('G', TRUE),
      'url' => $form_state['values']['table']['new']['url'],
      'weight' => $form_state['values']['table']['new']['weight'],
      'ldate' => REQUEST_TIME,
    ))
  ->execute();
  }
  unset($form_state['values']['new']);
  foreach ($form_state['values']['table'] as $key => $link) {
    if ($key != 'new' && ($link['weight'] != $link['oldweight'])) {
      // TODO Please review the conversion of this statement to the D7 database API syntax.
      /* db_query("UPDATE {linklist_maintainer_goals} SET weight = %d WHERE lid = %d", $link['weight'], $link['lid']) */
      db_update('linklist_maintainer_goals')
  ->fields(array(
        'weight' => $link['weight'],
      ))
  ->condition('lid', $link['lid'])
  ->execute();
    }
  }
  _linklist_maintainer_adjust_weights();
}
/**
 * Themes the form, including the table, for the linklist maintenance form
 * @param array $form
 * the $form array to be rendered
 * @return string
 * The rendered html output of the $form array
 */
function theme_linklist_maintainer_admin_linklist_form($variables) {
  $form = $variables['form'];
  $admin = user_access('administer linklist');
  $output .= drupal_render($form['filterlink']);
  $header = $form['#tableheader'];
  $rows = array();
  foreach (element_children($form['table']) as $key) {
    $row = array();
    $dontlinkit = $key == 'new';
    $row[]['data'] = drupal_render($form['table'][$key]['kw']);
    $row[] = array('data' => ($dontlinkit ? drupal_render($form['table'][$key]['url']) : l(drupal_render($form['table'][$key]['url']), 'admin/config/linklist-maintainer/targets/' . $key)) . drupal_render($form['table'][$key]['status']));
    $row[] = array('data' => drupal_render($form['table'][$key]['weight']), 'class' => 'weight centered');
    $row[] = array('data' => drupal_render($form['table'][$key]['ldate']), 'class' => 'date centered');
    $row[] = array('data' => drupal_render($form['table'][$key]['linkcount']), 'class' => 'centered');
    $row[] = array('data' => drupal_render($form['table'][$key]['sitecount']), 'class' => 'centered');
    if ($key == 'new') {
      $newrow = $row;
    }
    else {
      $editlink = l('details', 'admin/config/linklist-maintainer/link/details/' . $key);
      if ($admin) {
        $editlink .= ' | ' . l('edit', 'admin/config/linklist-maintainer/link/edit/' . $key, array('query' => drupal_get_destination()));
      }
      $row[] = array('data' => $editlink, 'class' => 'extras');
      $rows[] = $row;
    }
    unset($form['table'][$key]);
  }
  $rows[] = $newrow;
  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  unset($form['table']);
  $output .= drupal_render_children($form);
  drupal_add_css(drupal_get_path('module', 'linklist_maintainer') . '/linklist_maintainer_admin.css');
  return $output;
}

// LINK FORM
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_link_form($form, &$form_state, $lid = NULL, $edit = FALSE) {
  if ($link = db_fetch_array(db_query("SELECT lmg.*, COUNT(DISTINCT lms.pid) sitecount, COUNT(lms.lid) linkcount
    FROM {linklist_maintainer_goals} lmg
    LEFT JOIN {linklist_maintainer_stats} lms ON lmg.lid = lms.lid
    WHERE lmg.lid = :lmg.lid
    GROUP BY lmg.lid", array(':lmg.lid' => $lid)))) { // intentional assignment of $link
    $form = _linklist_maintainer_admin_link_form($link, $edit);
    if ($edit) {
      $form['url'] = array(
        '#type' => 'textfield',
        '#default_value' => $link['url'],
        '#size' => 50
      );
      $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Save'),
      );
      if ($link['linkcount'] > 1) {
        $form['instructions'] = array(
          '#value' => '<h4>' . t('Warning:') . '</h4><p>' . t('In general, you should not change the target url, since links
            are most effective if they are stable.  If you do change the url, you stand to lose the benefits of the @count
            links already made to the current url.  It is usually better to disable the link (which will freeze generation of new links)
            and then create a new link to a different url.', array('@count' => $link['linkcount'])) . '</p>',
        );
      }
    }
    else {
      $form['weight'] = array('#value' => $link['weight']);
    }
    $form['#validate'] = array('_linklist_maintainer_validate_link_form');
    if (!$_GET['destination']) {
      $form_state['#redirect'] = 'admin/config/linklist-maintainer/linklist';
    }

    if ($form_state['linklist_maintainer']['badurl']) {
      $form['submit']['#value'] = t('I have changed the url - try it again');
      $form['override'] = array(
        '#type' => 'submit',
        '#value' => t('I really want to create a bad link'),
      );
      $form['url']['#default_value'] = $form_state['linklist_maintainer'][$lid]['url'];
      $form['url']['#attributes'] = array('class' => 'error');
      $form['weight']['#default_value'] = $form_state['linklist_maintainer'][$lid]['weight'];
    }

    // TODO: phrase popularity, link distribution by site

    return $form;
  }
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_link_form_submit($form, &$form_state) {
  if ($form_state['values']['url'] != $form_state['values']['oldurl']) {
    // TODO Please review the conversion of this statement to the D7 database API syntax.
    /* db_query("UPDATE {linklist_maintainer_goals} SET url = '%s', weight = %d, ldate = %d WHERE lid = %d", $form_state['values']['url'], $form_state['values']['weight'], REQUEST_TIME, $form_state['values']['lid']) */
    db_update('linklist_maintainer_goals')
  ->fields(array(
      'url' => $form_state['values']['url'],
      'weight' => $form_state['values']['weight'],
      'ldate' => REQUEST_TIME,
    ))
  ->condition('lid', $form_state['values']['lid'])
  ->execute();
  }
  else {
    // TODO Please review the conversion of this statement to the D7 database API syntax.
    /* db_query("UPDATE {linklist_maintainer_goals} SET weight = %d WHERE lid = %d", $form_state['values']['weight'], $form_state['values']['lid']) */
    db_update('linklist_maintainer_goals')
  ->fields(array(
      'weight' => $form_state['values']['weight'],
    ))
  ->condition('lid', $form_state['values']['lid'])
  ->execute();
  }
  _linklist_maintainer_adjust_weights();
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function theme_linklist_maintainer_admin_link_form($variables) {
  $form = $variables['form'];
  $output = drupal_render($form['instructions']);
  $rows = array();
  foreach (
    array(
      'kw' => t('Link Phrase / Keyword'),
      'url' => t('Target URL'),
      'weight' => t('Weight'),
      'ldate' => t('Active Date'),
      'linkcount' => t('Link Count'),
      'sitecount' => t('Site Count'),
      'status' => t('Status'),
      'submit' => '',
      'override' => '',
    )
     as $key => $title
  ) {
    if ($form[$key]) {
      $rows[] = array(
        array('data' => $title, 'class' => 'leftcol'),
        array('data' => drupal_render($form[$key]))
      );
    }
  }
  $output .= theme('table', array('header' => array(), 'rows' => $rows, 'attributes' => array('class' => 'link-details')));
  $output .= drupal_render_children($form);
  drupal_add_css(drupal_get_path('module', 'linklist_maintainer') . '/linklist_maintainer_admin.css');
  return $output;
}

// PARTICIPANTS FORM
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_participants_form($form, &$form_state, $showall = FALSE) {
  switch ($showall) {
    case 'new':
      $where = 'WHERE !lmp.approved AND !lmp.blocked AND !lmp.banned AND !lmp.ignored';
      break;
    case 'flagged':
      $where = 'WHERE lmp.blocked OR lmp.banned';
      break;
    case 'approved':
      $where = 'WHERE lmp.approved AND !lmp.blocked AND !lmp.banned';
      break;
    case 'ignored':
      $where = 'WHERE lmp.ignored';
      break;
    case 'all':
      $where = '';
      break;
    default:
      $where = 'WHERE lmp.approved || !lmp.ignored';
      break;
  }
  drupal_set_title(t('Linklist Participants'));
  foreach (array('' => 'Default', 'new' => 'New Signups', 'flagged' => 'Blocked or Banned', 'approved' => 'Approved and Active', 'ignored' => 'Previously Ignored', 'all' => 'All Entries') as $arg => $t) {
    $filterlinks[] = l(t($t), "admin/config/linklist-maintainer/participants/$arg");
  }
  $form['filterlink']['#value'] = '<p><b>' . t('Filter list:') . '</b><br/>' . implode(' | ', $filterlinks) . '</p>';
  $form['#tableheader'] = array(
    array('data' => t('Server'), 'field' => 'lmp.server'),
    array('data' => t('Email'), 'field' => 'lmp.email'),
    array('data' => t('Active'), 'field' => 'lmp.pdate', 'class' => 'date centered'),
    array('data' => t('Updated'), 'field' => 'lmp.updated', 'class' => 'date centered'),
    array('data' => t('Links'), 'field' => 'linkcount', 'class' => 'centered', 'sort' => 'desc'),
    array('data' => t('Pages'), 'field' => 'pagecount', 'class' => 'centered'),
    array('data' => t('Approved'), 'field' => 'lmp.approved', 'class' => 'status centered'),
    array('data' => t('Blocked'), 'field' => 'lmp.blocked', 'class' => 'status centered'),
    array('data' => t('Banned'), 'field' => 'lmp.banned', 'class' => 'status centered'),
  );
  $q = pager_query("SELECT lmp.*, COUNT(lms.liid) linkcount, COUNT(DISTINCT lms.page) pagecount
    FROM {linklist_maintainer_participants} lmp LEFT JOIN {linklist_maintainer_stats} lms ON lmp.pid = lms.pid $where
    GROUP BY lmp.pid" . tablesort_sql($form['#tableheader']), 200, 0, "SELECT COUNT(*) FROM {linklist_maintainer_participants} lmp $where");



  $admin = user_access('administer linklist participants');
  while ($part = db_fetch_array($q)) {

    $unknown_ip = stripos($part['ip'], 'unknown-ip');
    $form['table'][$part['pid']]['pid'] = array('#type' => 'value', '#value' => $part['pid']);
    $form['table'][$part['pid']]['server'] = array('#value' => l($part['server'], $part['server']) . ($unknown_ip ? '<br/>' . $part['ip'] : ''));
    $form['table'][$part['pid']]['email'] = array('#value' => l($part['email'], 'mailto:' . $part['email']));
    $form['table'][$part['pid']]['pdate'] = array('#value' => $part['pdate'] ? format_date($part['pdate'], 'custom', 'j M Y') : '-');
    $form['table'][$part['pid']]['updated'] = array('#value' => $part['updated'] ? format_date($part['updated'], 'custom', 'j M Y') : '-');
    $form['table'][$part['pid']]['linkcount'] = array('#value' => $part['linkcount']);
    $form['table'][$part['pid']]['pagecount'] = array('#value' => $part['pagecount']);
    if ($admin) {
      $form['table'][$part['pid']]['approved'] = array('#type' => 'checkbox', '#default_value' => $part['approved'], '#disabled' => $part['approved']);
      if ($part['approved']) {
        $form['table'][$part['pid']]['blocked'] = array('#type' => 'checkbox', '#default_value' => $part['blocked']);
        $form['table'][$part['pid']]['banned'] = array('#type' => 'checkbox', '#default_value' => $part['banned']);
      }
      else {
        $form['table'][$part['pid']]['ignored'] = array('#type' => 'checkbox', '#default_value' => $part['ignored'], '#title' => t('Ignore'));
      }
    }
    else {
      $form['table'][$part['pid']]['approved'] = array('#value' => $part['approved'] ? 'Yes' : '');
      $form['table'][$part['pid']]['blocked'] = array('#value' => $part['blocked'] ? 'Yes' : '');
      $form['table'][$part['pid']]['banned'] = array('#value' => $part['banned'] ? 'Yes' : '');
    }

  }
  $form['table']['#tree'] = TRUE;
  $form['#theme'] = 'linklist_maintainer_admin_participants_form';
  if ($admin) {
    $form['submit'] = array('#type' => 'submit', '#value' => t('Save'));
  }
  $form['pager'] = array('#value' => theme('pager', array('tags' => NULL, 'element' => 0)));
  return $form;
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_participants_form_submit($form, &$form_state) {
  if (is_array($form_state['values']['table'])) {
    foreach ($form_state['values']['table'] as $key => $participant) {
      if ($participant['approved'] && !$participant['blocked'] && !$participant['banned']) {
        $participant['ip'] = '';
      }
      drupal_write_record('linklist_maintainer_participants', $participant, 'pid');
    }
  }
}
/**
 * Themes the form, including the table, for the linklist maintenance form
 * @param array $form
 * the $form array to be rendered
 * @return string
 * The rendered html output of the $form array
 */
function theme_linklist_maintainer_admin_participants_form($variables) {
  $form = $variables['form'];
  $output .= drupal_render($form['filterlink']);
  $header = $form['#tableheader'];
  $header[]['data'] = '';
  $rows = array();
  foreach (element_children($form['table']) as $key) {
    $approved = $form['table'][$key]['approved']['#default_value'];
    $class = stripos($form['table'][$key]['server']['#value'], 'unknown-ip') ? 'caution' : 'normal';
    $row = array();
    $row[]['data'] = drupal_render($form['table'][$key]['server']);
    $row[]['data'] = drupal_render($form['table'][$key]['email']);
    $row[] = array('data' => drupal_render($form['table'][$key]['active']), 'class' => 'date centered');
    $row[] = array('data' => drupal_render($form['table'][$key]['updated']), 'class' => 'date centered');
    $row[] = array('data' => drupal_render($form['table'][$key]['linkcount']), 'class' => 'centered');
    $row[] = array('data' => drupal_render($form['table'][$key]['pagecount']), 'class' => 'centered');
    $row[] = array('data' => drupal_render($form['table'][$key]['approved']), 'class' => 'goodbox centered');
    if ($approved) {
      $row[] = array('data' => drupal_render($form['table'][$key]['blocked']), 'class' => 'badbox centered');
      $row[] = array('data' => drupal_render($form['table'][$key]['banned']), 'class' => 'badbox centered');
    }
    else {
      $row[] = array('data' => drupal_render($form['table'][$key]['ignored']), 'class' => 'badbox centered', 'colspan' => 2);
    }
    $editlink = l(t('details'), 'admin/config/linklist-maintainer/participant/details/' . $key, array('query' => drupal_get_destination()));
    $row[] = array('data' => $editlink, 'class' => 'extras');
    $rows[] = array('data' => $row, 'class' => $class);
    unset($form['table'][$key]);
  }
  $output .= theme('table', array('header' => $header, 'rows' => $rows));
  unset($form['table']);
  $output .= drupal_render_children($form);
  drupal_add_css(drupal_get_path('module', 'linklist_maintainer') . '/linklist_maintainer_admin.css');
  return $output;
}

// PARTICIPANT DETAIL
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_participant_details($form, &$form_state, $pid = NULL) {
  global $user;
  if ($p = db_fetch_array(db_query("SELECT lmp.*, COUNT(lms.liid) linkcount, COUNT(DISTINCT lms.page) pagecount FROM
    {linklist_maintainer_participants} lmp LEFT JOIN {linklist_maintainer_stats} lms ON lmp.pid = lms.pid WHERE lmp.pid = :lmp.pid GROUP BY (lmp.pid)", array(':lmp.pid' => $pid)))) { // intentional assignment of $p
    $admin = user_access('administer linklist participants');
    $active = $p['approved'] && !$p['blocked'] && !$p['banned'] ? 'Active' : 'Inactive';
    $form = array();
    $form['pid'] = array('#type' => 'value', '#value' => $p['pid']);
    $form['#approvedflag'] = $p['approved'] ? 'status0' : 'status2';
    $form['#blockedflag'] = 'status' . ($p['banned'] ? 2 : $p['blocked']);
    $form['server'] = array('#value' => l($p['server'], $p['server']) . " <span class='server-status $active'>" . t($active) . "</span>");
    $form['email'] = array('#value' => l($p['email'], $p['email']));
    //$ips = unserialize($p['ip']); // for storing ip addresses - WILL BE USEFUL IF WE BRING BACK IP CHECKING
    if ($p['ip']) {
      $form['ip'] = array('#value' => $p['ip']);
    }
    $form['pdate'] = array('#value' => $p['pdate'] ? format_date($p['pdate'], 'custom', 'j M Y') : '-');
    $form['updated'] = array('#value' => $p['updated'] ? format_date($p['updated'], 'custom', 'j M Y') : '-');
    $form['linkcount'] = array('#value' => $p['linkcount']);
    $form['pagecount'] = array('#value' => $p['pagecount']);
    if ($admin) {
      $form['approved'] = array('#type' => 'checkbox', '#title' => t('Approved'), '#default_value' => $p['approved'], '#disabled' => $p['approved']);
      if ($p['approved']) {
        $form['blocked'] = array('#type' => 'checkbox', '#title' => t('Blocked'), '#default_value' => $p['blocked']);
        $form['banned'] = array('#type' => 'checkbox', '#title' => t('Banned'), '#default_value' => $p['banned']);
      }
      else {
        $form['ignored'] = array('#type' => 'checkbox', '#title' => t('Ignored'), '#default_value' => $p['ignored']);
      }
    }
    else {
      $form['approved'] = array('#value' => $p['approved'] ? '<span class="Active">' . t('Approved') . '</span>' : '<span class="Inactive">' . t('Unapproved') . '</span>');
      $form['blocked'] = array('#value' => $p['blocked'] ? '<span class="Inactive">' . t('Blocked') . '</span>' : '');
      $form['banned'] = array('#value' => $p['banned'] ? '<span class="Inactive">' . t('Banned') . '</span>' : '');
      $form['ignored'] = array('#value' => $p['ignored'] ? '<span class="Inactive">' . t('Ignored') . '</span>' : '');
    }

    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => $admin ? t('Save') : t('Back'),
    );


    if ($p['linkcount'] && module_exists('open_flash_chart_api')) {
      $total = db_query("SELECT COUNT(*) FROM {linklist_maintainer_stats} WHERE pid = :pid", array(':pid' => $p['pid']))->fetchField();
      $colors = array('#f4a460', '#d2691e', '#66cdaa', '#ffe4b5', '#b0c4de', '#bdb76b', '#ffa07a', '#ffdead', '#d8bfd8', '#a0522d');

      // Line chart of server growth
      $time_limit = strtotime('-2 years');
      $q = db_query("SELECT COUNT(liid) count, FROM_UNIXTIME(li_updated, '%Y%m') month FROM {linklist_maintainer_stats} WHERE pid = :pid AND li_updated > :li_updated GROUP BY month ORDER BY month DESC", array(':pid' => $p['pid'], ':li_updated' => $time_limit));

      // Pie charts of link characteristics
      $q = db_query_range("SELECT lmg.*, COUNT(lms.lid) linkcount
        FROM {linklist_maintainer_goals} lmg LEFT JOIN {linklist_maintainer_stats} lms ON lmg.lid = lms.lid
        WHERE lms.pid = :lms.pid GROUP BY lms.lid ORDER BY linkcount DESC", array(':lms.pid' => $p['pid']));
      $data['plinks']['title'] = 'Link Keywords';
      $data['ptargets']['title'] = 'Target Pages';
      $data['plinks']['title'] = 'Linked Phrases';
      while ($r = db_fetch_array($q)) {
        $data['plinks']['values'][] = $r['linkcount'];
        $data['plinks']['labels'][] = $r['kw'];
        $data['plinks']['count'] += $r['linkcount'];
      }
      $q = db_query_range("SELECT lmg.*, COUNT(lms.lid) linkcount
        FROM {linklist_maintainer_goals} lmg LEFT JOIN {linklist_maintainer_stats} lms ON lmg.lid = lms.lid
        WHERE lms.pid = :lms.pid GROUP BY lmg.url ORDER BY linkcount DESC", array(':lms.pid' => $p['pid']));
      $data['ptargets']['title'] = 'Link Targets';
      while ($r = db_fetch_array($q)) {
        if ($r['linkcount']) {
          $data['ptargets']['values'][] = $r['linkcount'];
          $data['ptargets']['labels'][] = $r['url'];
          $data['ptargets']['count'] += $r['linkcount'];
        }
      }
      foreach (array('plinks', 'ptargets') as $c) {
        if ($difference = $total - $data[$c]['count']) { // intentional assignment of $difference
          $data[$c]['values'][] = $total - $data[$c]['count'];
          $data[$c]['labels'][] = t('other');
        }
        $chart = new open_flash_chart_api();
        $chart->set_height(250);
        $chart->set_width(250);
        $chart->set_title(t($data[$c]['title']), 'font-size:24px; font-weight:bold;');
        $chart->pie(80, '#807cdc', 'display:none;', FALSE, 1);
        $chart->pie_values($data[$c]['values'], $data[$c]['values']);
        $chart->pie_slice_colours($colors);
        $chart->set_tool_tip('#label#');
        $chart->set_bg_colour('#ffffff');
        $chart_display = '<div class="piechart-wrapper">' . $chart->render() . '</div>';
        $key = '';
        foreach ($data[$c]['values'] as $i => $value) {
          $key .= "<div class='chart-key-wrapper'><div class='chart-key-color' style='background-color:{$colors[$i]};'>{$data[$c]['values'][$i]}</div>{$data[$c]['labels'][$i]}</div>";
        }
        $form[$c] = array('#value' => '<div class="piechart-outer">' . $chart_display . $key . '</div>');
        unset($chart);
      }
    }
    return $form;
  }

}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_participant_details_submit($form, &$form_state) {
  if (!is_null($form_state['values']['approved'])) {
    foreach (array('approved', 'blocked', 'banned', 'ignored', 'pid') as $field) {
      if (!is_null($form_state['values'][$field])) {
        $participant[$field] = $form_state['values'][$field];
      }
    }
    if ($form_state['values']['approved'] && !$form_state['values']['blocked'] && !$form_state['values']['banned']) {
      $participant['ip'] = '';
    }
    drupal_write_record('linklist_maintainer_participants', $participant, 'pid');
  }
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function theme_linklist_maintainer_admin_participant_details($variables) {
  $form = $variables['form'];
  $options .= "<div class='options {$form['#approvedflag']}'>" . drupal_render($form['approved']) . '</div>';
  $options .= "<div class='options {$form['#blockedflag']}'>" . drupal_render($form['blocked']) . drupal_render($form['banned']) . drupal_render($form['ignored']) . '</div>';
  $options .= '<div class="options">' . drupal_render($form['submit']) . '</div>';

  $rows = array();
  $rows[] = array(
    array('data' => 'Server', 'class' => 'leftcol'),
    array('data' => drupal_render($form['server'])),
    array('data' => $options, 'rowspan' => 7, 'class' => 'rightcol'),
  );
  foreach (
    array(
      'email' => t('Email'),
      'ip' => t('IP Addresses'),
      'pdate' => t('Date Active'),
      'updated' => t('Last Updated'),
      'linkcount' => t('Links'),
      'pagecount' => t('Pages'),
    )
     as $key => $title
  ) {
    if ($form[$key]) {
      $rows[] = array(
        array('data' => $title, 'class' => 'leftcol'),
        array('data' => drupal_render($form[$key]))
      );
    }
  }
  $output .= theme('table', array('header' => array(), 'rows' => $rows, 'attributes' => array('class' => 'link-details')));
  $output .= '<div class="charts" style="width:500px;">' . drupal_render($form['plinks']) . drupal_render($form['ptargets']) . '</div>';

  $output .= drupal_render_children($form);
  drupal_add_css(drupal_get_path('module', 'linklist_maintainer') . '/linklist_maintainer_admin.css');
  return $output;
}

// LINK TARGET REPORT
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_link_target_report($form, &$form_state, $lid = NULL) {

  $admin = user_access('administer linklist');

  // for individual urls
  if ($url = db_query("SELECT url FROM {linklist_maintainer_goals} WHERE lid = :lid", array(':lid' => $lid))->fetchField()) { // intentional assignment of $url
    drupal_set_title(t('Target Detail : @url', array('@url' => $url)), PASS_THROUGH);
    $form['#tableheader'] = array(
      array('data' => t('Keyword'), 'field' => 'lmg.url', 'colspan' => 2),
      array('data' => t('Links'), 'field' => 'lcount', 'class' => 'centered', 'sort' => 'desc'),
      array('data' => t('Participants'), 'field' => 'pcount', 'class' => 'centered'),
      array('data' => t('Weight'), 'field' => 'kwcount', 'class' => 'centered'),
    );
    $q = pager_query("SELECT lmg.kw, lmg.kw details, COUNT(lms.liid) lcount, COUNT(DISTINCT lms.pid) pcount, lmg.weight, lmg.lid FROM
      {linklist_maintainer_goals} lmg LEFT JOIN {linklist_maintainer_stats} lms ON lmg.lid = lms.lid WHERE lmg.url = '%s' GROUP BY lmg.lid" . tablesort_sql($form['#tableheader']),
      200, 0, "SELECT COUNT(*) FROM {linklist_maintainer_goals} WHERE url = '%s'", $url);
    $form['url'] = array('#value' => l(t('visit this target'), $url, array('absolute' => TRUE)));
    while ($r = db_fetch_array($q)) {
      $editlink = l('details', 'admin/config/linklist-maintainer/link/details/' . $r['lid'], array('query' => drupal_get_destination()));
      if ($admin) {
        $editlink .= ' | ' . l('edit', 'admin/config/linklist-maintainer/link/edit/' . $r['lid'], array('query' => drupal_get_destination()));
      }
      $r['details'] = $editlink;
      $form['table'][$r['lid']] = _linklist_maintainer_admin_target_form($r);
      if ($admin) {
        $form['table'][$r['lid']]['weight'] = array('#type' => 'textfield', '#size' => 5, '#default_value' => $r['weight']);
      }
    }
    if ($admin) {
      $form['submit'] = array('#type' => 'submit', '#value' => t('Save'));
      $form['oldurl'] = array('#type' => 'value', '#value' => $url);
      $form['changes'] = array('#type' => 'fieldset', '#title' => t('Change target (experts only!)'), '#collapsible' => TRUE, '#collapsed' => TRUE);
      $form['changes']['warning'] = array('#value' => '<div class="warning">' . t('PLEASE READ: Normally it is better to leave existing
        link goals pointed at their current targets, set their weights to 0, and create new links; otherwise, you will lose all the links to the old page.
        The only time you should use this feature is when you REALLY do not want any traffic to the old url anymore.  Importantly, if a target page moves to a new url,
        you should generally leave these links pointing to the old location, and have that server do a 301 redirect to the new location.') . '</div>');
      $form['changes']['newurl'] = array('#type' => 'textfield', '#default_value' => $url, '#title' => t('Change target URL for all links'));
    }
  }
  // for the list of urls
  else {
    drupal_set_title('Link Targets');
    $form['#tableheader'] = array(
      array('data' => t('Target URL'), 'field' => 'lmg.url', 'colspan' => 2),
      array('data' => t('Keywords'), 'field' => 'kwcount', 'class' => 'centered'),
      array('data' => t('Total Weight'), 'field' => 'weight', 'class' => 'centered'),
      array('data' => t('Links'), 'field' => 'lcount', 'class' => 'centered', 'sort' => 'desc'),
      array('data' => t('Participants'), 'field' => 'pcount', 'class' => 'centered'),
    );
    $q = pager_query("SELECT lmg.lid, lmg.url, lmg.lid details, COUNT(DISTINCT lmg.lid) kwcount,
      (SELECT SUM(lmgw.weight) FROM {linklist_maintainer_goals} lmgw WHERE lmgw.url = lmg.url) total_weight, COUNT(lms.liid) lcount, COUNT(DISTINCT lms.pid) pcount
      FROM {linklist_maintainer_goals} lmg LEFT JOIN {linklist_maintainer_stats} lms ON lmg.lid = lms.lid GROUP BY lmg.url" . tablesort_sql($form['#tableheader']),
      200, 0, "SELECT COUNT(DISTINCT url) FROM {linklist_maintainer_goals}");
    while ($r = db_fetch_array($q)) {
      $id = $r['lid'];
      unset($r['lid']);
      $status = _linklist_maintainer_linkcheck($r['url']);
      $status = $status['success'] ? " - <span class='linkcheck-good'>ok ({$status['response']})</span>" : " - <span class='linkcheck-bad'>bad ({$status['response']})</span>";
      $r['details'] = l(t('[visit]'), $r['url']);
      $r['url'] = l($r['url'], 'admin/config/linklist-maintainer/targets/' . $id, array('query' => drupal_get_destination()));
      $form['table'][$id] = _linklist_maintainer_admin_target_form($r);
      $form['table'][$id]['url']['#value'] .= $status;
    }
  }
  $form['table']['#tree'] = TRUE;
  $form['#theme'] = 'linklist_maintainer_admin_link_target_report';
  $form['pager'] = array('#value' => theme('pager', array('tags' => NULL, 'element' => 0)));
  drupal_add_css(drupal_get_path('module', 'linklist_maintainer') . '/linklist_maintainer_admin.css');
  return $form;
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_link_target_report_validate($form, &$form_state) {
  if (!$form_state['values']['newurl']) {
    form_set_error('newurl', t('a url is required'));
  }
  elseif ($form_state['values']['newurl'] != $form_state['values']['oldurl']) {
    $linkcheck = _linklist_maintainer_linkcheck($form_state['values']['newurl']);
    if (!$linkcheck['success']) {
      form_set_error('newurl', t('That url was unavailable (http response "@response").  <br/>
        <span style="font-size:smaller;">If you REALLY want to add a bad link, you must do it from the link editing forms.</span>', array('@response' => $linkcheck['response'])), 'error');
    }
  }
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function linklist_maintainer_admin_link_target_report_submit($form, &$form_state) {
  if ($form_state['values']['newurl'] != $form_state['values']['oldurl']) {
    foreach ($form_state['values']['table'] as $lid => $link) {
      $link['url'] = $form_state['values']['newurl'];
      $link['adj_weight'] = $link['weight'];
      $link['ldate'] = REQUEST_TIME;
      drupal_write_record('linklist_maintainer_goals', $link, 'lid');
    }
  }
  else {
    foreach ($form_state['values']['table'] as $lid => $link) {
      $link['adj_weight'] = $link['weight'];
      drupal_write_record('linklist_maintainer_goals', $link, 'lid');
    }
  }
}
/**
 * @todo Please document this function.
 * @see http://drupal.org/node/1354
 */
function theme_linklist_maintainer_admin_link_target_report($variables) {
  $form = $variables['form'];
  $output = drupal_render($form['url']);
  $rows = array();
  foreach (element_children($form['table']) as $lid) {
    $row = array();
    $first = 1;
    foreach (element_children($form['table'][$lid]) as $field) {
      if ($field != 'lid') {
        $class = $field . ($first ? ' first' : ' centered');
        $row[] = array('data' => drupal_render($form['table'][$lid][$field]), 'class' => $class);
        $first = 0;
      }
    }
    $rows[] = $row;
  }
  $output .= theme('table', array('header' => $form['#tableheader'], 'rows' => $rows));
  $output .= drupal_render_children($form);
  return $output;
}

// HELPER FUNCTIONS

/**
 * Helper function to generate a form array for a single link
 *
 * @param array $link
 * An array of values for a single link, drawn from the linklist_maintainer_goals and linklist_maintainer_stats tables<br/><code>SELECT lmt.*, COUNT(DISTINCT lms.pid) sitecount, COUNT(lms.lid) linkcount FROM {linklist_maintainer_targets} lmt LEFT JOIN {linklist_maintainer_stats} lms GROUP BY lmt.lid</code>
 * @return array
 * The $form array for a single link
 */
function _linklist_maintainer_admin_link_form($link = array()) {

  $access = user_access('administer linklist');

  $weight = array(
    '#type' => 'textfield',
    '#default_value' => $link ? $link['weight'] : 5,
    '#size' => 5,
  );
  if ($link) {
    $form['kw'] = array(
      '#value' => check_plain($link['kw']),
    );
    $form['url'] = array(
      '#value' => check_plain($link['url'])
    );
    $status = _linklist_maintainer_linkcheck($link['url']);
    $form['status'] = array(
      '#value' => $status['success'] ? " - <span class='linkcheck-good'>ok ({$status['response']})</span>" : " - <span class='linkcheck-bad'>bad ({$status['response']})</span>",
    );
    $form['weight'] = $access ? $weight : array('#value' => $link['weight']);
    $form['oldurl'] = array(
      '#type' => 'value',
      '#value' => $link['url'],
    );
    $form['oldweight'] = array(
      '#type' => 'value',
      '#value' => $link['weight'],
    );
    $form['lid'] = array(
      '#type' => 'value',
      '#value' => $link['lid'],
    );
    foreach (array('ldate', 'linkcount', 'sitecount') as $key) {
      $form[$key] = array(
        '#value' => $link[$key],
      );
    }
    $form['ldate']['#value'] = $form['ldate']['#value'] ? format_date($form['ldate']['#value'], 'custom', 'j M Y') : '-';
  }
  elseif ($access) {
    $form['kw'] = array(
      '#type' => 'textfield',
      '#size' => 25,
    );
    $form['url'] = array(
      '#type' => 'textfield',
      '#size' => 50,
    );
    $form['weight'] = $weight;
  }

  $form['#element_validate'][] = '_linklist_maintainer_validate_link_form';
  return $form;
}
function _linklist_maintainer_validate_link_form($element, &$form_state) {
  if ($element['kw']['#value'] || $element['url']['#value']) {
    $lid = $element['lid'] ? $element['lid']['#value'] : 'new';
    if (!$element['kw']['#value']) {
      form_error($element['kw'], t('both a link phrase and a url are required'));
    }
    if (!$element['url']['#value']) {
      form_error($element['url'], t('both a link phrase and a url are required'));
    }
    if ($element['url']['#value'] != $element['oldurl']['#value']) {

      $linkcheck = _linklist_maintainer_linkcheck($element['url']['#value']);
      if (!$linkcheck['success']) {
        if ($form_state['clicked_button']['#value'] == t('I really want to create a bad link')) {
          drupal_set_message(t('You just created a bad link.'), 'error');
        }
        else {
          drupal_set_message(t('That url was unavailable (http response "' . $linkcheck['response'] . '").  <br/>
            <span style="font-size:smaller;">If you REALLY want to add a bad link, click the appropriate button below.</span>'), 'error');
          $form_state['rebuild'] = TRUE;
          $form_state['linklist_maintainer']['badurl'] = TRUE;
          $form_state['linklist_maintainer'][$lid] = array(
            'kw' => $element['kw']['#value'],
            'url' => $element['url']['#value'],
            'weight' => $element['weight']['#value']
          );
        }
      }
    }
    if (!is_numeric($element['weight']['#value'])) {
      form_error($element['weight'], t('weight must be numeric'));
    }
  }
}

function _linklist_maintainer_admin_target_form($target) {
  $admin = user_access('administer linklist');
  foreach ($target as $key => $value) {
    $form[$key] = array(
      '#value' => $value,
    );
  }

  if ($form['lid']) { // if this is a single link
    $form['lid']['#type'] = 'value';
    $form['#element_validate'][] = '_linklist_maintainer_validate_target_form';
  }

  return $form;
}
function _linklist_maintainer_validate_target_form($element, &$form_state) {
  if (!is_numeric($element['weight']['#value'])) {
    form_error($element['weight'], t('weight must be numeric'));
  }
}

/**
 * Checks a single link to determine its status
 *
 * @param string $url
 * An absolute or relative url (relative urls will be expanded to local urls and checked)
 * @return array
 *   <code>
 *  'url' => the absolute url for the link
 *  'response' => http header (e.g. 200, 301, 404)
 *  'success' => a boolean indication of TRUE if response = 200 or 301; FALSE otherwise
 *  </code>
 */
function _linklist_maintainer_linkcheck($url) {

  $url = url($url, array('absolute' => TRUE));

  $ch = @curl_init($url);
  @curl_setopt($ch, CURLOPT_HEADER, TRUE);
  @curl_setopt($ch, CURLOPT_NOBODY, TRUE);
  @curl_setopt($ch, CURLOPT_FOLLOWLOCATION, FALSE);
  @curl_setopt($ch, CURLOPT_RETURNTRANSFER, TRUE);
  $status = array();
  $response = @curl_exec($ch);
  preg_match('/HTTP\/.* ([0-9]+) .*/', $response, $status);
  curl_close($ch);

  return array(
    'url' => $url,
    'response' => $status[1],
    'success' => in_array($status[1], array(200, 301)),
  );

}

/*
 * This function is for retrieving stats for a better administrative interface.
 *
 *
 function _linklist_maintainer_linkstats() {
 $weight = db_fetch_object(db_query("SELECT MAX(weight) max, MIN(weight) min FROM {linklist_maintainer_goals} WHERE weight > 0"));
 $steps = round(($weight->max - $weight->min) / 2);
 if ($steps < 2) return;
 $weight->min--;
 $step = ($weight->max - $weight->min) / 10;
 $sql = "SELECT COUNT(*) FROM {linklist_maintainer_goals} WHERE weight >= %d AND weight <= %d";
 $y = $weight->max;
 for ($x = $weight->max - $step; $x > $weight->min; $x -= $step) {
 $y = $x;
 $data[] = db_result(db_query($sql, round($x), round($y)));
 }
 if (module_exists('open_flash_chart_api')) {

 }
 }
 //*/

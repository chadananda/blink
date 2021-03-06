<?php
/**
 Plugin Name: bLinks
 Plugin URI: http://labs.uswebdev.net
 Description: Automatically links your blog posts other web sites.
 Author: Daniel Jones
 Version: 1.1
 Author URI: http://labs.uswebdev.net
 */

include 'blink.class.php';
include_once(ABSPATH . WPINC . '/class-IXR.php');

/**
 * BLink plugin class implimentation
 */
class BLinkCollaborationPlugin {

  function BLinkCollaborationPlugin() { //BL constructor
    $this->db_check();
  }

  /**
   * Adds links to content
   */
  function createLinks($content = '') {
  // get the post ID
    global $post;
    $postID = $post->ID;
    // create the blink class instance
    $blinkClass = new blink();
    // get the maximum number of links per page
    //$options = $this->getAdminOptions();
    $admin_options = get_option('BlinkPluginAdminOptions');
    $maxLinks =  $admin_options['maxLinks'];
    $cssClass = get_option('css_class');
    // get an array of current links
    $current_links = $this->getCurrentLinks($postID);
    $current_links_gids = array();
    foreach($current_links as $link) {
      $current_links_gids[] = $link['gid'];
    }
    $existing_link_count = count($current_links);
    // no pre-existing links
    if ($existing_link_count == 0) {
    // get goal links
      $goal_links = $this->getLinkGoals();
      // add them to content
      if (count($goal_links)) {$currentLinks = $blinkClass->markup_text(&$content, $goal_links, $maxLinks, $cssClass);}
      // if any were added update database
      if (count($currentLinks)>0) {
        $this->insertBlinks($currentLinks, $postID);
      }
    } else { // if links allready exist
    // add existing links
      $stillCurrentLinks = $blinkClass->markup_text(&$content, $current_links, $existing_link_count, $cssClass);
      $stillCurrentLinks_gids = array();
      foreach ($stillCurrentLinks as $link) {
        $stillCurrentLinks_gids[] = $link['gid'];
      }
      $existing_link_count = count($stillCurrentLinks);
      // delete removed links
      $deleted_links = array();
      foreach ($current_links as $key => $link) {
        if (!in_array($link['gid'], $stillCurrentLinks_gids)) {
          $deleted_links[] = $link;
        }
      }
      if (count($deleted_links)) $this->removeBlinks($deleted_links, $postID);
      // check if there is room for more links
      if ($existing_link_count < $maxLinks) {
      // get an array of all goal links
        $linkGoals = $this->getLinkGoals();
        //remove current links from link goals
        // build lookup hash
        $cur_gids = array();
        foreach ($stillCurrentLinks as $link) {$cur_gids[] = $link['gid']; }
        // strip out the exising links
        foreach ($linkGoals as $key=>$link) {if (in_array($link['gid'], $cur_gids)) {unset($linkGoals[$key]);}}
        //new link amounts
        $links_available = $maxLinks - $existing_link_count;
        // add new links
        $new_links = $blinkClass->markup_text(&$content, $linkGoals, $links_available, $cssClass);
        // update the existing links in the table
        // combine the old links with the newly added links
        $this->insertBlinks($new_links,$postID);
      }
    }
    return $content;
  }

  /**
   * adds links to the blink_links table
   * BLINK_LINKS
   * liid : auto-increment field/key
   * link_uid : uniqid generated locally
   * gid : fk to BLINK_GOALS
   * pid : Blog Post ID
   * url_override : not currently in use
   * li_updated : date this link was created (unix time stamp)
   */
  function insertBlinks($links, $postID) {
  // update Existing Links table
    if (is_array($links) && count($links) > 0) {
      global $wpdb;
      $table_name = $wpdb->prefix . "blink_links";

      foreach ($links as $link) {
        $sql = "INSERT INTO $table_name (link_uid, gid, pid, li_updated)
                VALUES ('" . uniqid('l', true) . "', " . $link['gid'] . ", $postID, UNIX_TIMESTAMP(now()) )";
        $wpdb->query($sql);
      }
    }
  }

  /**
   * removes dead links from the blink_links table
   */
  function removeBlinks($links, $postID) {
  // setup database access
    if (is_array($links) && count($links) > 0) {
      global $wpdb;
      $table_name = $wpdb->prefix . "blink_links";
      // remove links
      foreach ($links as $link) {
        $sql = "DELETE FROM  $table_name
          WHERE gid = '" . $link['gid'] . 
        "' AND pid = '" . $postID . "'";
        $wpdb->query($sql);
      }
    }
  }

  /**
   * retrieves previously used links for a blog post
   *
   * $results : array( array(
   *	kw => 'human rights',
   *	weight => 35,
   *  url => 'http://www.myblog.com/',
   *	gid => 15
   *	),);
   */
  function getCurrentLinks($postID) {
    global $wpdb;
    $table_name = $wpdb->prefix . "blink_goals g";
    $table2_name = $wpdb->prefix . "blink_links e";
    $sql = "SELECT g.kw, g.weight_local weight, g.page url, g.gid
            FROM  $table_name , $table2_name
            WHERE g.gid = e.gid
            AND e.pid =  $postID";
    $results = $wpdb->get_results($sql,ARRAY_A);
    return $results;
  }

  function getLinkGoals() {
    global $wpdb;
    // add prefix to table name
    $table_name = $wpdb->prefix . "blink_goals";
    // setup select statement
    $sql = "SELECT  kw, weight_local weight, page url, gid FROM $table_name WHERE weight > 0 AND weight_local > 0";
    $results = $wpdb->get_results( $sql, ARRAY_A);
    return $results;
  }

 /*
  * Initialize Admin Settings
  */
  function init() {
    $this.getAdminOptions();
  }

  /**
   * Verify if DB tables have been created
   */
  function db_check() {
    global $wpdb;
    $table_name = $wpdb->prefix . "bl_maintainers";
    // if tables aren't installed then install them
    if ($wpdb->get_var("SHOW TABLES LIKE '$table_name'") != $table_name) {
      $this->db_install();
    }
  }

  /**
   * Create Database Tables
   *
   * BLINK_SERVERS
   * blsid : auto-increment field/key
   * server : URL to maintainer server
   * server_key : key pass to allow access to updates
   * server_weight : priority given to a maintainer server
   * last_update : last time update was run on this server (unix time stamp)
   *
   * BLINK_GOALS
   * gid : auto-increment field/key
   * goal_uid : uniqid provided by maintainer
   * blsid : fk to BLINK_SERVER table
   * kw : target word or phrase
   * page : target URL
   * weight : maintainer provided link priority
   * weight_local : combination of link and server weights
   * goal_source : not currently in use
   *
   * BLINK_LINKS
   * liid : auto-increment field/key
   * link_uid : uniqid generated locally
   * gid : fk to BLINK_GOALS
   * pid : Blog Post ID
   * url_override : not currently in use
   * li_updated : date this link was created (unix time stamp)
   *
   */
  function db_install () {
    global $wpdb;

    // BLINK_SERVERS
    $table_name = $wpdb->prefix . "blink_servers";
    $sql = "CREATE TABLE IF NOT EXISTS $table_name (
      blsid INTEGER AUTO_INCREMENT,
      server VARCHAR(255) NOT NULL,
      server_weight VARCHAR(255),
      last_successful_update INTEGER,
      last_update INTEGER,
      UNIQUE KEY blsid (blsid)
      );";
    $wpdb->query($sql);

    // Hack to get the default server in
    $sql = "INSERT INTO " . $table_name . "
      (`blsid`, `server`, `server_weight`,
      `last_successful_update`, `last_update`)
      VALUES (1, 'labs.uswebdev.net', '1', NULL, NULL); ";
    $wpdb->query($sql);

    // BLINK_GOALS
    $table_name = $wpdb->prefix . "blink_goals";
    $sql = "CREATE TABLE IF NOT EXISTS $table_name (
      gid INTEGER NOT NULL AUTO_INCREMENT,
      goal_uid VARCHAR(255),
      blsid INTEGER,
      kw VARCHAR(255),
      page VARCHAR(500),
      weight  INTEGER,
      weight_local INTEGER,
      goal_source VARCHAR(255),
      UNIQUE KEY gid (gid)
      );";
    $wpdb->query($sql);

    // BLINK_LINKS
    $table_name = $wpdb->prefix . "blink_links";
    $sql = "CREATE TABLE IF NOT EXISTS $table_name (
      liid INTEGER NOT NULL AUTO_INCREMENT,
      link_uid VARCHAR(255),
      gid INTEGER,
      pid INTEGER,
      url_override VARCHAR(255),
      li_updated INTEGER,
      UNIQUE KEY liid (liid));";
    $wpdb->query($sql);

    // create a blink_key to register the server
    if (!get_option('blink_key')) {
      $blink_key = uniqid('b', true);
      update_option('blink_key', $blink_key);
    }
  }

  /**
   * Retrieve configuration options and maintianers
   */
  function getAdminOptions() {
    $adminOptionsName = "BlinkPluginAdminOptions";
    global $wpdb;
    $servers_table = $wpdb->prefix . "blink_servers";
    $sql = "SELECT * FROM $servers_table ORDER BY server_weight";
    $results = $wpdb->get_results( $sql,ARRAY_A);
    // set array of settings to default values
    $blNewSettings = array(
      'maxLinks' => 5,
      'exclude_blinks' => '',
      'css_class' => '',
      'blink_servers' => $results
    );
    // check if admin options have been set
    if (get_option($adminOptionsName)) {
      $currentSettings = get_option($adminOptionsName);
      $blNewSettings['maxLinks'] = $currentSettings['maxLinks'];
      $blNewSettings['css_class'] = $currentSettings['css_class'];
      $blNewSettings['exclude_blinks'] = $currentSettings['exclude_blinks'];
    }
    // update options with current or default settings
    update_option($adminOptionsName,$blNewSettings);
    return $blNewSettings;
  }

  /**
   * generates the admin page
   */
  function printAdminPage() {
    require ('adminPage.php');
    adminPage($this);
  }

  function update_maintainers($server) {
  // this will need to be reworked to support multiple servers
    global $wpdb;
    $servers_table = $wpdb->prefix . "blink_servers";
    $currentServers = $wpdb->get_results("SELECT * FROM $servers_table",ARRAY_A);
    if (count($server)>0 && count($currentServers)>0 ) {

      if ($server['server'] != $currentServers['server']) {
        $sql = "UPDATE $servers_table SET server= " . $currentServers['server'] .
              " SET last_update = null, SET last_successful_update = null
                WHERE server = '" . $server['server'] . "' ";
        $wpdb->query($sql);
      }
    } elseif (count($currentServers) == 0  && $server['server']) {
      $sql = "INSERT INTO $servers_table (server, server_weight)
              VALUES ('" .  $server['server'] . "', 1)";
      $wpdb->query($sql);
    }
    update_option('maxLinks',$server['maxLinks']);
    $this->checkServers();
  }

  /**
   * unregistered (update now)
   * failed_update (check every 1 hr)
   * pending_approval  (check every 4 hrs)
   * successful_update (check every 7days)
   */
  function checkServers() {

    global $wpdb;
    $table_name = $wpdb->prefix . "blink_servers";
    $sql = "SELECT *, UNIX_TIMESTAMP(now()) now  FROM $table_name";
    $servers =  $wpdb->get_results($sql,ARRAY_A);

    // delete orphaned links from trashed posts
    $blink_links = $wpdb->prefix . "blink_links";
    $delete_sql = "DELETE bl FROM $blink_links bl, $wpdb->posts p WHERE bl.pid = p.id AND p.post_status != 'publish' AND p.post_parent = 0";
    $wpdb->query($delete_sql);

    // check server's status
    if (count($servers)>0) {
      foreach ($servers as $server) {
      //				if(!$server['last_update']){
      //					// unregistered
      //					$this->regServer($server);
      //				} elseif (!$server['last_successful_update']) {
      //					// pending approval (14400 == 4hr)
      //					$time_diff = (int)$server['now'] - (int)$server['last_update'];
      //if($time_diff > 0 ){	$this->updateServer($server);}
      //				} elseif ($server['last_update'] > $server['last_successful_update']){
      //echo 'failed';
      //					// failed_update (3600 == 1hr)
      //					$time_diff = (int)$server['now'] - (int)$server['last_update'];
      //if($time_diff > 0 ){
      //echo 'check 3';
      //						$this->updateServer($server);
      //					}
      //				} elseif ($server['last_update'] == $server['last_successful_update']){
      //					// successful update (601200 == 1wk)
      //					$time_diff = (int)$server['now'] - (int)$server['last_update'];
      //if($time_diff > 0 ){
      //echo 'check 4';
        $this->updateServer($server);
      //					}
      //				}
      }
    }
  }

  function updateServer($server) {
    global $wpdb;
    $servers_table = $wpdb->prefix . "blink_servers";
    $goals_table = $wpdb->prefix . "blink_goals";

    // call RPC and get the new goals
    $blsid = $server['blsid'];
    $links = $this->getBlinks($blsid);
    $updateLinks = $this->callRPC($blsid);
    $newGoals = $updateLinks['linklist'];

    if (count($newGoals) > 0) {
    // update server as successfull upgrade
      $sql = "UPDATE $servers_table
        SET last_successful_update = UNIX_TIMESTAMP(now())
        WHERE blsid = " . $server['blsid'];
      $wpdb->query($sql);

      // get existing goal records
      $sql = "SELECT  * FROM $goals_table WHERE blsid = $blsid";
      $oldGoals = $wpdb->get_results( $sql,ARRAY_A);

      if (count($oldGoals) > 0) {
      // convert existing goals to array with goal_uid as key
        foreach ($oldGoals as $goal) {$currentGoals[$goal['goal_uid']] = $goal;}

        // update existing goals
        foreach ($newGoals as $newGoal) {
          if ($currentGoal[$newGoal['goal_uid']]) {
            if ( ($newGoal['weight'] != $currentGoal['weight'])  ||
                 ($newGoal['url'] != $currentGoal['page']) ||
                 ($newGoal['kw'] != $currentGoal['kw']) ) {
              $sql = "UPDATE $goals_table SET weight = " . $newGoal['weight'] .
                ", SET page =  '" . $newGoal['url'] .
                ", SET kw =  '" . $newGoal['kw'] .
                "' WHERE goal_uid = " . $newGoal['goal_uid'] ;
              $wpdb->query($sql);
            }
          }
        }

        //downgrade removed goals
        foreach ($currentGoals as $currentGoal) {
          if (!$newGoals[$currentGoal['goal_uid']]) {
            $wpdb->query("UPDATE $goals_table SET weight=0, weight_local=0 WHERE goal_uid=" . $currentGoal['goal_uid']);
          }
        }
      }

      //add new goals
      $this->addNewGoals($blsid, $newGoals);

      //update weight_local
      $sql ="UPDATE $goals_table g,
            ( SELECT g1.blsid, g1.server_weight / g2.sum_weight sv_weight
              FROM $servers_table g1,
               (SELECT blsid, sum(server_weight)  sum_weight
                FROM $servers_table
                GROUP BY blsid) g2
              WHERE g1.blsid = g2.blsid
            ) svr,
            ( SELECT g1.gid, g1.blsid, g1.weight/g2.sum_weight goal_weight
              FROM $goals_table g1,
               (SELECT blsid, sum(weight) sum_weight
                FROM $goals_table
                GROUP BY blsid) g2
              WHERE g1.blsid = g2.blsid
            ) gl
            SET weight_local = svr.sv_weight * gl.goal_weight *1000
            WHERE g.blsid = gl.blsid
            AND   g.gid =  gl.gid
            AND   g.blsid = svr.blsid";

      $wpdb->query($sql);
    }
  }

  function addNewGoals($blsid, $newGoals) {
    global $wpdb;
    $goals_table = $wpdb->prefix . "blink_goals";

    $sql = "SELECT  * FROM $goals_table WHERE blsid = $blsid";
    $oldGoals = $wpdb->get_results( $sql,ARRAY_A);
    if (count($oldGoals)>0) {
      foreach ($oldGoals as $goal) {$currentGoals[$goal['goal_uid']] = $goal;}
    }
    foreach ($newGoals as $newGoal) {
      if (!$currentGoals[$newGoal['goal_uid']]) {
      //insert goal record
        $sql = "INSERT INTO $goals_table (goal_uid, blsid, kw, page, weight) VALUES ('" .
          $newGoal['goal_uid'] . "', $blsid, '" . $newGoal['kw'] . "', '" .
          $newGoal['url'] . "', " .$newGoal['weight'] . ")";
        $wpdb->query($sql);
      }
    }
  }

  /**
   *
   * Retrieves an array of exsiting links
   * sutible for RPC updates
   */
  function getBlinks($blsid) {
    global $wpdb;
    $links_table = $wpdb->prefix . "blink_links l ";
    $post_table = $wpdb->prefix . "posts p ";
    $goal_table = $wpdb->prefix . "blink_goals g ";
    $sql = "SELECT l.link_uid, g.goal_uid, l.li_updated, p.guid
            FROM $links_table, $post_table,  $goal_table
            WHERE  g.gid = l.gid
            AND p.id = l.pid
            AND g.blsid = $blsid ";
    $blinks = $wpdb->get_results($sql, ARRAY_A);
    $results = array();
    if (count($blinks) > 0) {
      foreach ($blinks as $blink) {
        $results[$blink['link_uid']] = array(
          'link_uid'=>$blink['link_uid'],
          'goal_uid'=>$blink['goal_uid'],
          'page'=>$blink['guid'],
          'li_updated'=>$blink['li_updated']
        );
      }
    }
    return $results;
  }

  /**
   * register new server
   */
  function regServer($server) {
    $key = get_option('blink_key');
    $blsid = $server['blsid'];
    $this->callRPC($blsid);
    global $wpdb;
    $table_name = $wpdb->prefix . "blink_servers";
    // update the last update field
    $sql = "UPDATE " . $table_name .
          " SET last_update = UNIX_TIMESTAMP(now()) " .
          " WHERE blsid =  " . $blsid ;
    $wpdb->query($sql);
  }

  /**
   *
   * Comunicate with maintianer module
   *
   * @global <type> $wpdb
   * @param <type> $blsid
   * @return <type>
   */
  function callRPC($blsid) {
    $links = $this->getBlinks($blsid);
    // todo: change how we handle version tracking
    $version = '1.1wp';
    global $wpdb;
    $table_name = $wpdb->prefix . "blink_servers";
    $sql = "SELECT * FROM " .  $table_name . " where blsid = " . $blsid ;
    $blinks =  $wpdb->get_results( $sql,ARRAY_A);
    // get the RPC server's URL
    $server = $blinks[0]['server'];
    $server = $this->getURL($server);
    // get the blog's URL
    $blog = get_option('siteurl'); //'http://' . $this->getURL();
    $key = get_option('blink_key');
    $mail = get_option('admin_email');
    $result = array();
    // create the RPC client object
    $client =  new IXR_Client($server, '/xmlrpc.php');
    // either update or register
    if (count($links) > 0) {
    //update
      if (! $client->query('linklistMaintainer.getLinklist', $blog, $mail, $key, $version, $links)) {
        die($message = 'Something went wrong - ' . $client->getErrorCode() . ' : ' . $client->getErrorMessage());
        $result['success'] = false;
        $result['message'] = $message;
      } else {
        $result = $client->getResponse();
      }
    } else {
    //register
      if (! $client->query('linklistMaintainer.getLinklist', $blog, $mail, $key)) {
        die($message = 'Something went wrong - ' . $client->getErrorCode() . ' : ' . $client->getErrorMessage());
      } else {
        $result = $client->getResponse();
      }
    }
    return $result;
  }

  function getURL($url = '') {
    if (!$url) $url = get_option('siteurl');
    return preg_replace('|^[^:]+://([^:/]+).*|', '$1', $url, 1);
  }

} // end class

/**
 * create an instance of our bl class
 */
if (class_exists("BLinkCollaborationPlugin")) {
  $bl_collaboration = new BLinkCollaborationPlugin();
}

/**
 * create actions and filters
 */
if (isset($bl_collaboration)) {
//Actions
//add_action('wp_head',array(&$bl_collaboration,'db_check'),1);
// creates the database
  add_action('activate_blink-wp/blink-wp.php',array(&$bl_collaboration,'db_check'),1);
  // sets default options so the admin page can load
  add_action('activate_blink-wp/blink-wp.php', array(&$bl_collaboration,'init'));

  add_action('admin_menu','bLinkCollaboration_ap');
  add_action('admin_menu', array(&$bl_collaboration,'checkServers'),12);

  //Filters
  add_filter('the_content',array(&$bl_collaboration,'createLinks'));
}

/**
 * Initialize the admin panel
 */
function bLinkCollaboration_ap() {
  global $bl_collaboration;
  if (!isset($bl_collaboration)) {
    return;
  }
  add_options_page("Blink Settings", "BLink Settings", 9, __FILE__, array(&$bl_collaboration,'printAdminPage'));
}


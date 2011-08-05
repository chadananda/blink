<?php
	function adminPage(&$blClass){
		// load current settings
		$devOptions = $blClass->getAdminOptions();
		$server = $devOptions['blink_servers'][0];

		// update settings code
		if (isset($_POST['update_blink_settings'])) {
			if (isset($_POST['maxLinks'])) {
				$devOptions['maxLinks'] = $_POST['maxLinks'];
				update_option(maxLinks, $devOptions['maxLinks']);
			}
			if (isset($_POST['css_class'])) {
				update_option('css_class', $_POST['css_class']);
			}
			if (isset($_POST['server'])) {
				//$devOptions['maintainers'][$maintainer_keys[0]]['server'] = $_POST['server_0'];
				$server['server'] = $_POST['server'];
			}
			if (isset($_POST['exclude_links'])) {
				$devOptions['exclude_blinks'] = $_POST['exclude_links'];
			}
			$blClass->update_maintainers($server);
			update_option('BlinkPluginAdminOptions', $devOptions);
		}
		echo " <form method=post action=" . $_SERVER[REQUEST_URI] . ">";
		echo "<h2>BLink Settings</h2>
					<h3>Remote Server</h3>
					<div class=remote-servers style=\"border-style: solid; border-width:
            2px; max-width: 415px; border-color: LightGrey\">
					<table border=0>
					<tr><td>Server</td><td></td></tr>
					<tr><td><input type=text name=server size=45 value=\"labs.uswebdev.net\" READONLY>
					</td></tr></table>";
					//$server['server'] .	
		// max links dropdown
		$maxLinks = $devOptions['maxLinks'];
		$linkCount = 11;
		//$cssClass = get_option('css_class');
		//echo 'hello';
		//echo $cssClass;
		echo "<table border=0>
					<tr><td width=230 align=left>CSS Class </td><td> Max Links Per Page </td></tr>
					<tr><td align=left >
					<input type=text name=css_class value='" . get_option('css_class') .
					"' size=15></td>

<td align=right><select name=maxLinks>";
		for($x=1; $x < $linkCount; $x++){
			if($maxLinks == $x ) {
				echo "<option value=" . $x . " selected=1>". $x . "</option>";
			} else {
				echo "<option value=" . $x . ">" . $x . "</option>";
			}
		}

		echo "</select> </td></tr></table></div>";
		echo "<br>"; 
		//  exclude paths
    //	echo "<h3>Excluded Paths</h3>" .
    //			"<textarea name=exclude_links cols=45 rows=10 >" .
    //	$devOptions['exclude_blinks'] .
    //	" </textarea> <br><br>  ";
		//  apply button
//    echo	"<div style=\"max-width: 400px; text-align: right \">" .
//					"<button type=\"clear\" name=\"clear_blink_settings\">Drop Tables</button>" .
//					"</div>";
//    echo "<br>";
		echo	"<div style=\"max-width: 400px; text-align: right \">" .
					"<button type=\"submit\" name=\"update_blink_settings\">Apply</button>" .
					"</div></form>"	;
	}// end printAdminPage


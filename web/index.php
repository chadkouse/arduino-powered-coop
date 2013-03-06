<html>
<head><title>Coop Controller</title>
<script language="javascript">
function sendAjax(url)
{
  //use an image request since it's fire-and-forget
  //otherwise you are bound to CORS restrictions
  function remove() { 
        img = null; 
    }
    var img = new Image();
    img.onerror = remove;
    img.onload = remove;
    img.src = url;  
}
</script>
</head>
<body>

<?php

$coop_ip = "10.0.1.98";
$show_cam_snapshot = TRUE;
$snapshot_uri = "http://10.0.1.103/video.cgi?user=guest&pwd=guest";

function curlCall($url)
{
  /* Initializing curl */
  $ch = curl_init($url);

  // Configuring curl options
  $options = array(
  CURLOPT_RETURNTRANSFER => true,
  CURLOPT_HTTPHEADER => array('Content-type: application/json') ,
  );

  // Setting curl options
  curl_setopt_array( $ch, $options );

  // Getting results
  $result =  curl_exec($ch); // Getting jSON result string"
  return $result;
}

if (isset($_POST["cmd"]))
{
  //run a command
  $cmd = $_POST["cmd"];
  if ($cmd == "setTime")
  {
    if (isset($_POST["on1_hour"]) && isset($_POST["on1_min"]))
    {
      $newSeconds = intval($_POST["on1_hour"]) * 60 *60;
      $newSeconds += intval($_POST["on1_min"]) * 60;
      $output = json_decode(curlCall(sprintf("http://%s/?on1=%f", $coop_ip, $newSeconds)), TRUE);
      printf("<span style='color:darkgreen'>On1 set to %02d:%02d</span><br />", $_POST["on1_hour"], $_POST["on1_min"] );
    }
    if (isset($_POST["on2_hour"]) && isset($_POST["on2_min"]))
    {
      $newSeconds = intval($_POST["on2_hour"]) * 60 *60;
      $newSeconds += intval($_POST["on2_min"]) * 60;
      $output = json_decode(curlCall(sprintf("http://%s/?on2=%f", $coop_ip, $newSeconds)), TRUE);
      printf("<span style='color:darkgreen'>On2 set to %02d:%02d</span><br />", $_POST["on2_hour"], $_POST["on2_min"] );
    }
  }
  else if ($cmd == "doorOn")
  {
      $output = json_decode(curlCall(sprintf("http://%s/?door_on=1", $coop_ip)), TRUE);
      echo "<span style='color:darkgreen'>Door Activated!</span><br />";
  }
}


$json = json_decode(curlCall(sprintf("http://%s/?getInfo", $coop_ip)), TRUE);

$on1 = $json["on1"];
$on2 = $json["on2"];
$curTime = $json["time"];

function decodeTimeSec($seconds) 
{
  $hours = getHoursFromSeconds($seconds);
  $minutes = getMinutesFromSeconds($seconds);
  return sprintf("%d:%02d", $hours, $minutes);
}

function getHoursFromSeconds($seconds)
{
  return floor($seconds/(60*60));
}

function getMinutesFromSeconds($seconds)
{
  $hours = getHoursFromSeconds($seconds);
  return ($seconds-($hours*60*60))/60;
}

function getOptions($start, $end, $val)
{
  for ($i = $start; $i <= $end; $i++)
  {
    printf("<option value=\"%d\"%s>%d</option>", $i, $i==$val ? " SELECTED" : "", $i); 
  }
}

function getHourOptions($val=null)
{
  getOptions(0, 23, $val);
}

function getMinuteOptions($val=null)
{
  getOptions(0, 59, $val);
}

?>

Current Timer Settings:<br/>
Current Time: <?= decodeTimeSec($curTime); ?><br/>
On 1: <?= decodeTimeSec($on1); ?><br/>
On 2: <?= decodeTimeSec($on2); ?><br/>
<br/><hr/><br/>

<form method="POST">
<input type="hidden" name="cmd" value="setTime">
New On1 Time <select name="on1_hour"><?= getHourOptions(getHoursFromSeconds($on1)); ?></select> <select name="on1_min"><?= getMinuteOptions(getMinutesFromSeconds($on1)); ?></select><br/>
New On2 Time <select name="on2_hour"><?= getHourOptions(getHoursFromSeconds($on2)); ?></select> <select name="on2_min"><?= getMinuteOptions(getMinutesFromSeconds($on2)); ?></select><br/>
<input type="submit" />
</form>
<br/><hr/><br/>

<form method="POST">
<input type="hidden" name="cmd" value="doorOn">
<input type="submit" value="Activate the door NOW" />
</form>

<?php

if ($show_cam_snapshot)
?>
<img src="<?= $snapshot_uri; ?>" />
<br >
<a href="#" onclick="sendAjax('http://guest:guest@10.0.1.103/decoder_control.cgi?command=95');return false;">IR On</a>&nbsp;&nbsp;&nbsp;&nbsp;
<a href="#" onclick="sendAjax('http://guest:guest@10.0.1.103/decoder_control.cgi?command=94');return false;">IR Off</a>&nbsp;&nbsp;&nbsp;&nbsp;
<?php

?>
</body>
</html>

<?php

$data = file('results.dat');
$results = Array();
$teams = Array();
$rankings = Array();

for ($i = 0; $i < sizeof($data); ++$i)
{
  $res = split(" ", $data[$i]);
  if (sizeof($res) == 4) {
    if (array_search($res[0], $teams) === FALSE) {
      array_push($teams, $res[0]);
    }
    if (array_search($res[1], $teams) === FALSE) {
      array_push($teams, $res[1]);
    }
    $res[0] = array_search($res[0], $teams);
    $res[1] = array_search($res[1], $teams);
    $results[sizeof($results)] = $res;
  }
}


$rankings = Array();
for ($t = 0; $t < sizeof($teams); ++$t)
{
  $rankings[$t] = Array();
  $rankings[$t][0] = Array(0, 1500);
}

for ($i = 0; $i < sizeof($results); ++$i)
{
  $res = $results[$i];
  $t1 = $res[0];
  $t2 = $res[1];
  
  $r1 = $rankings[$t1][sizeof($rankings[$t1]) - 1][1];
  $r2 = $rankings[$t2][sizeof($rankings[$t2]) - 1][1];
  
  $q1 = pow(10, $r1 / 400);
  $q2 = pow(10, $r2 / 400);
  
  $e1 = $q1 / ($q1 + $q2);
  $e2 = $q2 / ($q1 + $q2);
  
  $s1 = $res[2] > $res[3] ? 1 : ($res[2] == $res[3] ? .5 : 0);
  $s2 = 1 - $s1;
  
  $r1new = round($r1 + 32 * ($s1 - $e1));
  $r2new = round($r2 + 32 * ($s2 - $e2));
  
  //print_r($res);
  //echo "r: $r1 $r2, q: $q1 $q2, e: $e1 $e2, s: $s1 $s2, $r1new $r2new\n";
  array_push($rankings[$t1], Array($i + 1, $r1new));
  array_push($rankings[$t2], Array($i + 1, $r2new));
}

$cmd2 = "echo 'set terminal pngcairo transparent size 400, 300 font \"Sans,8\" \\nset output \"ratings.png\" \\nunset xtics \\nset key top left \\nplot 1500 notitle lt 0";
for ($t = 0; $t < sizeof($teams); ++$t)
{
  $fd = fopen("/tmp/$teams[$t].rating", "w");
  //fwrite($fd, ($rankings[$t][0][0])." ".$rankings[$t][0][1]."\n");
  fwrite($fd, ($rankings[$t][1][0] - 1)." ".$rankings[$t][0][1]."\n");
  for ($r = 1; $r < sizeof($rankings[$t]); ++$r) {
    fwrite($fd, $rankings[$t][$r][0]." ".$rankings[$t][$r][1]."\n");
  }
  $cmd = "echo 'set terminal pngcairo transparent size 100, 25 \\nset output \"$teams[$t].png\" \\nunset tics \\nunset key \\nplot \"/tmp/$teams[$t].rating\" with lines' > /tmp/plot.gp";
  system($cmd);
  system("gnuplot /tmp/plot.gp");
  $cmd = "echo 'set terminal pngcairo transparent size 100, 25 \\nset output \"$teams[$t].png\" \\nunset tics \\nunset key \\nplot \"/tmp/$teams[$t].rating\" with lines' > /tmp/plot.gp";
  system($cmd);
  system("gnuplot /tmp/plot.gp");
  $cmd2 .= ", \"/tmp/$teams[$t].rating\" with lines title \"$teams[$t]\" lw 3";
}
$cmd2 .= "' > /tmp/plot.gp";
system($cmd2);
system("gnuplot /tmp/plot.gp");


?>

<html>
  <head>
    <title>SimControl Tournament</title>
    <style>
    body {
      background-color: #222;
    }
    
    body, td {
      font-family: sans-serif;
    }
    
    h1 {
      color: #000;
      width: 800px;
      background-color: #d70;
    }
    
    h2 {
      color: d70;
      width: 800px;
      background-color: #000;
      margin-bottom: 0px;
    }
    
    table {
      background-color: #eee;
    }
    
    td, th {
      padding-right: 20px;
      vertical-align: top;
    }
    
    th {
      border-bottom: solid 1px #000;
      color: #420;
    }
    </style>
  </head>
  <body>
  <h1>SimControl Tournament</h1>
  <h2>Team Ratings</h2>
  <table width="800">
  <tr><td>
  <table>
    <thead>
      <th>Team</th><th>Rating</th><th></th>
    </thead>
    <tbody>
    <?
      for ($t = 0; $t < sizeof($teams); ++$t)
      {
        echo "<tr><td>".$teams[$t]."</td><td>".$rankings[$t][sizeof($rankings[$t]) - 1][1]."</td><td><img src=\"$teams[$t].png\"/></td></tr>\n";
        $rankings[$t][0] = Array(0, 1500);
      }
    ?>
    </tbody>
  </table>
  </td><td>
    <img src="ratings.png"/>
  </td>
  </tr></table>
  <h2>Matches</h2>
  <table width="800">
    <thead>
      <th>Team 1</th><th>Team 2</th><th>Score 1</th><th>Score 2</th>
    </thead>
    <tbody>
    <?
    for ($i = 0; $i < sizeof($results); ++$i)
    {
      $res = $results[$i];
      $bg = $i % 2 ? "#ddd" : "#fff";
      echo "<tr style=\"background-color: $bg;\"><td>".$teams[$res[0]]."</td><td>".$teams[$res[1]]."</td><td align=\"center\">".$res[2]."</td><td align=\"center\">".$res[3]."</td></tr>\n";
    }
    ?>
  </body>
</html>

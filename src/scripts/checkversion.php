<?php

function ver_num($version)
{
    return $version[0] * 10000 + $version[1] * 100 + $version[2];
}

function ver_str($version)
{
    return sprintf("%d.%d.%d", $version[0], $version[1], $version[2]);
}

function get_client_wc_version()
{
    if (!isset($_SERVER["HTTP_USER_AGENT"]))
        return false;

    if (sscanf($_SERVER["HTTP_USER_AGENT"], "wc-ng/%d.%d.%d",
               $major, $minor, $patch) != 3)
        return false;

    return ver_num(array($major, $minor, $patch));
}

$CHANGELOG = array(
    "0.7.2" => array("first open source release!",
                     "please see http://sauerworld.org/forum => mods => wc-ng ".
                     "for more")
);

$REVOKEDCERTS = array(
    // "39:A0:ED:2E:D6:10:69:B7:C6:55:E1:9E:FC:87:0B:BF:25:7A:C4:2D"
);

$CURRENT_VERSION = ver_num(explode(".", key($CHANGELOG)));
$client_version = get_client_wc_version();

if ($client_version === false)
    die("Wrong User-Agent!");

$revokedcerts = null;
$numrevokedcerts = count($REVOKEDCERTS);

for ($i = 0; $i < $numrevokedcerts; ++$i)
{
    $revokedcerts .= $REVOKEDCERTS[$i];
    if ($i >= 1 && $i < $numrevokedcerts-1)
        $revokedcerts += " ";
}

if ($revokedcerts !== null)
    echo $revokedcerts . "\n";

if ($client_version < $CURRENT_VERSION)
{
    $changes = null;

    foreach ($CHANGELOG as $version => $changed)
    {
        if (ver_num(explode(".", $version)) <= $client_version)
            break;

        $changes .= $version . "\n" . implode($changed, "\n") . "\n";
    }

    if ($changes !== null)
        echo $changes;
}

?> 

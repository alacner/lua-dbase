<?php
// Path to dbase file
$db_path = "/tmp/base/shbase.dbf";

// Open dbase file
$dbh = dbase_open($db_path, 0)
  or die("Error! Could not open dbase database file '$db_path'.");

// Get column information
$column_info = dbase_get_header_info($dbh);

// Display information
print_r($column_info);
?> 

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

if ($dbh) {
  $record_numbers = dbase_numrecords($dbh);
  for ($i = 1; $i <= $record_numbers; $i++) {
      $row = dbase_get_record_with_names($dbh, $i);
		print_r($row);
      $row = dbase_get_record($dbh, $i);
		print_r($row);
		break;
  }
      $row = dbase_get_record($dbh, 9);
		print_r($row);
}
?> 

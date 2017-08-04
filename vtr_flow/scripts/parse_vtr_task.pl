#!/usr/bin/perl

###################################################################################
# This script is used to extract and verify statistics of one or more VTR tasks.
#
# Usage:
#	parse_vtr_task.pl <task_name1> <task_name2> ... [OPTIONS]
#
# Options:
# 	-l <task_list_file>: Used to provide a test file containing a list of tasks
#   -create_golden:  Will create/overwrite the golden results with those of the
#						most recent execution
#   -check_golden:  Will verify the results of the most recent execution against
#						the golden results for each task and report either a
#						[Pass] or [Fail]
#   -parse_qor:  	Used for the purposes of parsing quality of results of the
#					most recent execution.
#   -calc_geomean:  Used for the purposes of computing quality of results geomeans 
# 					of the most recent execution.
#
# Authors: Jason Luu and Jeff Goeders
###################################################################################

use strict;
use Cwd;
use File::Spec;
use File::Copy;
use List::Util;
use Math::BigInt;
use POSIX qw/strftime/;

# Function Prototypes
sub trim;
sub parse_single_task;
sub pretty_print_table;
sub summarize_qor;
sub calc_geomean;
sub check_golden;
sub expand_user_path;
sub get_important_file;

# Get Absolute Path of 'vtr_flow
Cwd::abs_path($0) =~ m/(.*vtr_flow)/;
my $vtr_flow_path = $1;

my $run_prefix = "run";

# Parse Input Arguments
my @tasks;
my @task_files;
my $token;
my $create_golden = 0;
my $check_golden  = 0;
my $parse_qor 	  = 1;  # QoR file is parsed by default; turned off if 
						# user does not specify QoR parse file in config.txt
my $calc_geomean  = 0;  # QoR geomeans are not computed by default;
my $override_exp_id       = 0;
my $revision;
my $verbose       = 0;
my $pretty_print_results = 1;

while ( $token = shift(@ARGV) ) {

	# Check for a task list file
	if ( $token =~ /^-l(.+)$/ ) {
		push( @task_files, expand_user_path($1) );
	}
	elsif ( $token eq "-l" ) {
		push( @task_files, expand_user_path( shift(@ARGV) ) );
	}
	elsif ( $token eq "-create_golden" ) {
		$create_golden = 1;
	}
	elsif ( $token eq "-check_golden" ) {
		$check_golden = 1;
	}
	elsif ( $token eq "-parse_qor" ) {
		$parse_qor = 1;
	}
	elsif ( $token eq "-calc_geomean" ) {
		$calc_geomean = 1;
	}
	elsif ( $token eq "-run") {
	    $override_exp_id = shift(@ARGV);
    }
	elsif ( $token eq "-revision" ) {
		$revision = shift(@ARGV);
	}
	elsif ( $token eq "-v" ) {
		$verbose = 1;
	}
	elsif ( $token =~ /^-/ ) {
		die "Invalid option: $token\n";
	}
	# must be a task name
	else {
		if ( $token =~ /(.*)\/$/ ) {
			$token = $1;
		}
		push( @tasks, $token );
	}
}

# Read Task Files
foreach (@task_files) {
	open( FH, $_ ) or die "$! ($_)\n";
	while (<FH>) {
		push( @tasks, $_ );
	}
	close(FH);
}

my $num_golden_failures = 0;
foreach my $task (@tasks) {
	chomp($task);
	my $failed = parse_single_task($task);
    if($failed) {
        $num_golden_failures += 1; 
    }
}

if ($calc_geomean) {
	summarize_qor;
	calc_geomean;
}

exit $num_golden_failures;

sub parse_single_task {
	my $task_name = shift;
	(my $task_path = $task_name) =~ s/\s+$//;

    # first see if task_name is the task path
	if (! -e "$task_path/config/config.txt") {
	    ($task_path = "$vtr_flow_path/tasks/$task_name") =~ s/\s+$//;
    }

	open( CONFIG, "<$task_path/config/config.txt" )
	  or die "Failed to open $task_path/config/config.txt: $!";
	my @config_data = <CONFIG>;
	close(CONFIG);

	my @circuits;
	my $parse_file;
	my $qor_parse_file;
        my $second_parse_file;
	my @archs;
        my $counter = 0;
	foreach my $line (@config_data) {

		# Ignore comments
		if ( $line =~ /^\s*#.*$/ or $line =~ /^\s*$/ ) { next; }

        #Trim off a line-ending comment
        $line =~ s/#.*$//;

		my @data  = split( /=/, $line );
		my $key   = trim( $data[0] );
		my $value = trim( $data[1] );

		if ( $key eq "circuit_list_add" ) {
			push( @circuits, $value );
		}
		elsif ( $key eq "arch_list_add" ) {
			push( @archs, $value );
		}
		elsif ( $key eq "parse_file" ) {
                        if ($counter eq 1){
                                    #second time parse file
                                    $second_parse_file = expand_user_path($value);
                                    $counter = 0;
                                    #don't need to check golden, only compare between two files
                                    $check_golden = 0;
                        }
                        else{
                            $parse_file = expand_user_path($value);
                            $counter = $counter + 1;
                        }
		}
		elsif ( $key eq "qor_parse_file" ) {
			$qor_parse_file = expand_user_path($value);
		}
	}

	# PARSE CONFIG FILE
	if ( $parse_file eq "" ) {
		die "Task $task_name has no parse file specified.\n";
	}
	$parse_file = get_important_file($task_path, $vtr_flow_path, $parse_file);

        if ($second_parse_file){
            $second_parse_file = get_important_file($task_path, $vtr_flow_path, $second_parse_file);
	}

	# Get Max Run #
	opendir(DIR, $task_path);
	my @folders = readdir(DIR);
	closedir(DIR);
	# QOR PARSE CONFIG FILE
	if ( $qor_parse_file eq "" ) {
		print "Task $task_name has no QoR parse file specified. Skipping QoR.\n";
		$parse_qor = 0;
		$calc_geomean = 0;
	}
	else {
	    $qor_parse_file = get_important_file($task_path, $vtr_flow_path, $qor_parse_file);
    }

    my $exp_id = 0;
    if($override_exp_id != 0) {
        #explicitely specified via -run parameter
        $exp_id = $override_exp_id;
    } else {
        # haven't explicitely specified via -run parameter
        $exp_id = last_exp_id(${task_path});
    }
	
	my $run_path = "$task_path/${run_prefix}${exp_id}";

	my $first = 1;
	open( OUTPUT_FILE, ">$run_path/parse_results.txt" );
	foreach my $arch (@archs) {
		foreach my $circuit (@circuits) {
			system(
				"$vtr_flow_path/scripts/parse_vtr_flow.pl $run_path/$arch/$circuit $parse_file > $run_path/$arch/$circuit/parse_results.txt"
			);
			open( RESULTS_FILE, "$run_path/$arch/$circuit/parse_results.txt" );
            # first line is heading
			my $output = <RESULTS_FILE>;
			if ($first) {
				print OUTPUT_FILE "arch\tcircuit\t$output";
				$first = 0;
			}
            # second line is actual value
			my $output = <RESULTS_FILE>;
			close(RESULTS_FILE);
			print OUTPUT_FILE $arch . "\t" . $circuit . "\t" . $output;
		}
	}
	close(OUTPUT_FILE);

    #parse the second file the same way as the first if checking rr graph
    if($second_parse_file){
        my $run_path = "$task_path/${run_prefix}${exp_id}";

	my $first = 1;
	open( OUTPUT_FILE, ">$run_path/parse_results_2.txt" );
	foreach my $arch (@archs) {
		foreach my $circuit (@circuits) {
			system(
				"$vtr_flow_path/scripts/parse_vtr_flow.pl $run_path/$arch/$circuit $second_parse_file > $run_path/$arch/$circuit/parse_results_2.txt"
			);
			open( RESULTS_FILE, "$run_path/$arch/$circuit/parse_results_2.txt" );
            # first line is heading
			my $output = <RESULTS_FILE>;
			if ($first) {
				print OUTPUT_FILE "arch\tcircuit\t$output";
				$first = 0;
			}
            # second line is actual value
			my $output = <RESULTS_FILE>;
			close(RESULTS_FILE);
			print OUTPUT_FILE $arch . "\t" . $circuit . "\t" . $output;
		}
	}
	close(OUTPUT_FILE);
    }

    if ($pretty_print_results) {
        pretty_print_table("$run_path/parse_results.txt");
        if ($second_parse_file) {
            pretty_print_table("$run_path/parse_results_2.txt");
        }
    }

	if ($parse_qor) {
		my $first = 1;
		open( OUTPUT_FILE, ">$run_path/qor_results.txt" );
		foreach my $arch (@archs) {
			foreach my $circuit (@circuits) {
				system(
					"$vtr_flow_path/scripts/parse_vtr_flow.pl $run_path/$arch/$circuit $qor_parse_file > $run_path/$arch/$circuit/qor_results.txt"
				);
				open( RESULTS_FILE, "$run_path/$arch/$circuit/qor_results.txt" );
				my $output = <RESULTS_FILE>;
				if ($first) {
					print OUTPUT_FILE "arch\tcircuit\t$output";
					$first = 0;
				}
				my $output = <RESULTS_FILE>;
				close(RESULTS_FILE);
				print OUTPUT_FILE $arch . "\t" . $circuit . "\t" . $output;
			}
		}
		close(OUTPUT_FILE);
	}

	if ($create_golden) {
		copy( "$run_path/parse_results.txt",
			"$run_path/../config/golden_results.txt" );
	}

        if ($second_parse_file){
            #don't check with golden results, just check the two files
            return check_two_files ( $task_name, $task_path, $run_path, "$run_path/parse_results.txt", "$run_path/parse_results_2.txt", 0);
            
        }

	if ($check_golden) {
        #Returns 1 if failed
            if ($second_parse_file eq ""){
                return check_two_files( $task_name, $task_path, $run_path, "$run_path/parse_results.txt", "$task_path/config/golden_results.txt", $check_golden);
            }
	}

    return 0; #Pass
}

sub summarize_qor {

	##############################################################
	# Set up output file
	##############################################################

	my $first = 1;
	
	my $task = @tasks[0];
	(my $task_path = "$vtr_flow_path/tasks/$task") =~ s/\s+$//;
	
	my $output_path = $task_path;
	my $exp_id = last_exp_id($task_path);

	if ( ( ( $#tasks + 1 ) > 1 ) | ( -e "$task_path/../task_list.txt" ) ) {
		$output_path = "$task_path/../";
	}
	if ( !-e "$output_path/task_summary" ) {
		mkdir "$output_path/task_summary";
	}
	if ( -e "$output_path/task_summary/${run_prefix}${exp_id}_summary.txt" ) {
	}
	open( OUTPUT_FILE, ">$output_path/task_summary/${run_prefix}${exp_id}_summary.txt" );
	
	##############################################################
	# Append contents of QoR files to output file
	##############################################################

	foreach my $task (@tasks) {
		chomp($task);
		($task_path = "$vtr_flow_path/tasks/$task") =~ s/\s+$//;
		$exp_id = last_exp_id($task_path);
		(my $run_path = "$task_path/${run_prefix}${exp_id}") =~ s/\s+$//;

		open( RESULTS_FILE, "$run_path/qor_results.txt" );
		my $output = <RESULTS_FILE>;

		if ($first) {
			print OUTPUT_FILE "task_name\t$output";
			$first = 0;
		}
		
		while ($output = <RESULTS_FILE>) {
			print OUTPUT_FILE $task . "\t" . $output;
		}
		close(RESULTS_FILE);
	}
	close(OUTPUT_FILE);
}

sub calc_geomean {

	##############################################################
	# Set up output file
	##############################################################

	my $first = 0;

	my $task = @tasks[0];
	(my $task_path = "$vtr_flow_path/tasks/$task") =~ s/\s+$//;
	
	my $output_path = $task_path;
	my $exp_id = last_exp_id($task_path);

	if ( ( ( $#tasks + 1 ) > 1 ) | ( -e "$task_path/../task_list.txt" ) ) {
		($output_path = "$task_path/../") =~ s/\s+$//; 
	}
	if ( !-e "$output_path/qor_geomean.txt" ) {
		open( OUTPUT_FILE, ">$output_path/qor_geomean.txt" );
		$first = 1;
	}
	else {
		open( OUTPUT_FILE, ">>$output_path/qor_geomean.txt" );
	}
	
	##############################################################
	# Read summary file
	##############################################################

	my $summary_file = "$output_path/task_summary/${run_prefix}${exp_id}_summary.txt";

	if ( !-r $summary_file ) {
		print "[ERROR] Failed to open $summary_file: $!";
		return;
	}
	open( SUMMARY_FILE, "<$summary_file" );
	my @summary_data = <SUMMARY_FILE>;
	close(SUMMARY_FILE);

	my $summary_params = shift @summary_data;
	my @summary_params = split( /\t/, trim($summary_params) );
	
	if ($first) {
		# Hack - remove unwanted labels
		my $num = 4;
		while ($num) {
			shift @summary_params;
			--$num;
		}
		print OUTPUT_FILE "run";
		my @temp = @summary_params;
		while ( $#temp >= 0 ) {
			my $label = shift @temp;
			print OUTPUT_FILE "\t" . "$label";
		}
		print OUTPUT_FILE "\t" . "date" . "\t" . "revision";
		$first = 0;
	}
	else {
	}

	print OUTPUT_FILE "\n${exp_id}";

	##############################################################
	# Compute & write geomean to output file
	##############################################################

	my $index = 4;
	my @summary_params = split( /\t/, trim($summary_params) );
	
	while ( $#summary_params  >= $index ) {
		my $geomean = 1; my $num = 0;
		foreach my $line (@summary_data) {
			my @first_file_line = split( /\t/, $line );
			if ( trim( @first_file_line[$index] ) > 0 ) {
				$geomean *= trim( @first_file_line[$index] );
				$num++;
			}
		}
		if ($num) {
			$geomean **= 1/$num;
			print OUTPUT_FILE "\t" . "${geomean}";
		}
		else {
			print OUTPUT_FILE "\t" . "-1";
		}
		$index++;
	}
	my $date = strftime( '%D', localtime );
	print OUTPUT_FILE "\t" . "$date" . "\t" . "$revision";
	close(OUTPUT_FILE);
}

sub max {
    my $x = shift;
    my $y = shift;

    return ($x < $y) ? $y : $x;
}

sub pretty_print_table {
    my $file_path = shift;


    #Read the input file
    my @file_data;
    open(INFILE,"<$file_path");
    while(<INFILE>) {
        chomp;
        push(@file_data, [split /\t/])
    }

    #Determine the maximum column width for pretty formatting
    my %col_widths;
    for my $row (0 .. $#file_data) {
        for my $col (0 .. $#{$file_data[$row]}) {

            my $col_width = length $file_data[$row][$col];

            #Do we have a valid column width?
            if (not exists $col_widths{$col}) {
                #Initial width
                $col_widths{$col} = $col_width;
            } else {
                #Max width
                $col_widths{$col} = max($col_widths{$col}, $col_width);
            }

        }
    }

    #Write out in pretty format
    open(OUTFILE,">$file_path");
    for my $row (0 .. $#file_data) {
        for my $col (0 .. $#{$file_data[$row]}) {
            printf OUTFILE "%-*s", $col_widths{$col}, $file_data[$row][$col];

            if($col != $#{$file_data[$row]}) {
                printf OUTFILE "\t";
            }
        }
        printf OUTFILE "\n";
    }
    close(OUTFILE);

}

sub last_exp_id {
	my $path = shift;
	my $num = 0;
    my $run_id = "";
    my $run_id_no_pad = "";
    do {
		++$num;
        $run_id = sprintf("%03d", $num);
        $run_id_no_pad = sprintf("%d", $num);
    } while ( -e "$path/${run_prefix}${run_id}" or -e "$path/${run_prefix}${run_id_no_pad}");
    --$num;
    $run_id = sprintf("%03d", $num);
    $run_id_no_pad = sprintf("%d", $num);

    if( -e "$path/${run_prefix}${run_id}" ) {
        return $run_id;
    } elsif (-e "$path/${run_prefix}${run_id_no_pad}") {
        return $run_id_no_pad;
    }

    die("Unknown experiment id");
} 

sub check_two_files {
	my $task_name = shift;
	my $task_path = shift;
	my $run_path  = shift;
        my $first_test_file_dir = shift;
        my $second_test_file_dir = shift;
        my $is_golden = shift;

    #Did this check pass?
	my $failed = 0;

    print "$task_name...";
    print "\n" if $verbose;

	# Code to check the results of the two files
	(my $test_file_1 = "$first_test_file_dir") =~ s/\s+$//;
	(my $test_file_2 = "$second_test_file_dir") =~ s/s+$//;

	my $pass_req_file;
	open( CONFIG_FILE, "$task_path/config/config.txt" );
	my $lines = do { local $/; <CONFIG_FILE>; };
	close(CONFIG_FILE);

	# Search config file
	if ( $lines =~ /^\s*pass_requirements_file\s*=\s*(\S+)\s*$/m ) { }
	else {
		print
		  "[ERROR] No 'pass_requirements_file' in task configuration file ($task_path/config/config.txt)\n";
        $failed = 1;
		return $failed;
	}

	my $pass_req_filename = $1;

	# Search for pass requirement file
	$pass_req_filename = expand_user_path($pass_req_filename);
	if ( -e "$task_path/config/$pass_req_filename" ) {
		$pass_req_file = "$task_path/config/$pass_req_filename";
	}
	elsif ( -e "$vtr_flow_path/parse/pass_requirements/$pass_req_filename" ) {
		$pass_req_file =
		  "$vtr_flow_path/parse/pass_requirements/$pass_req_filename";
	}
	elsif ( -e $pass_req_filename ) {
		$pass_req_file = $pass_req_filename;
	}
	else {
		print
		  "[ERROR] Cannot find pass_requirements_file.  Checked for $task_path/config/$pass_req_filename or $vtr_flow_path/parse/$pass_req_filename or $pass_req_filename\n";
        $failed = 0;
		return $failed;
	}

	my $line;

	my @first_test_data;
        my @second_test_data;
	my @pass_req_data;

	my @params;
	my %type;
	my %min_threshold;
	my %max_threshold;
	my %abs_diff_threshold;

	##############################################################
	# Read files
	##############################################################
	if ( !-r $test_file_2 ) {
		print "[ERROR] Failed to open $test_file_2: $!";
        $failed = 1;
		return $failed;
	}
	open( GOLDEN_DATA, "<$test_file_2" );
	@second_test_data = <GOLDEN_DATA>;
	close(GOLDEN_DATA);

	if ( !-r $pass_req_file ) {
		print "[ERROR] Failed to open $pass_req_file: $!";
        $failed = 1;
		return $failed;
	}
	open( PASS_DATA, "<$pass_req_file" );
	@pass_req_data = <PASS_DATA>;
	close(PASS_DATA);

	if ( !-r $test_file_1 ) {
		print "[ERROR] Failed to open $test_file_1: $!";
        $failed = 1;
		return $failed;
	}
	open( TEST_DATA, "<$test_file_1" );
	@first_test_data = <TEST_DATA>;
	close(TEST_DATA);

	##############################################################
	# Process and check all parameters for consistency
	##############################################################
	my $second_test_params = shift @second_test_data;
	my $first_test_params   = shift @first_test_data;

	my @second_test_params = split( /\t/, $second_test_params );    # get parameters of the second file results
	my @first_test_params = split( /\t/, $first_test_params );      # get parameters of the first file results

    my @second_test_params = map(trim($_), @second_test_params);
    my @first_test_params = map(trim($_), @first_test_params);

	# Check to ensure all parameters to compare are consistent
	foreach $line (@pass_req_data) {

		# Ignore comments
		if ( $line =~ /^\s*#.*$/ or $line =~ /^\s*$/ ) { next; }

		my @data = split( /;/, $line );
		my $name = trim( $data[0] );
		$type{$name} = trim( $data[1] );
		if ( trim( $data[1] ) eq "Range" ) {
			$min_threshold{$name} = trim( $data[2] );
			$max_threshold{$name} = trim( $data[3] );
		} elsif (trim( $data[1] ) eq "RangeAbs") {
			$min_threshold{$name} = trim( $data[2] );
			$max_threshold{$name} = trim( $data[3] );
			$abs_diff_threshold{$name} = trim( $data[4] ); #Third element is absolute threshold
        } elsif (trim( $data[1] ) eq "Equal") {
            #Pass
        } else {
			print "[ERROR] $name has no comparison check specified (e.g. Range, RangeAbs, Equal).\n";
            $failed = 1;
			return $failed;
        }

		#Ensure item is in the first file
		if ( !grep { $_ eq $name } @second_test_params ) {
                    if ($is_golden){
			print "[ERROR] $name is not in the golden results file.\n";
                    }else{
                        print "[ERROR] $name is not in the second parse file.\n";
                    }
                    $failed = 1;
			return $failed;
		}

		# Ensure item is in new results
		if ( !grep { $_ eq $name } @first_test_params ) {
		    if ($is_golden){
			print "[ERROR] $name is not in the generated results file.\n";
                    }else{
                        print "[ERROR] $name is not in the first parse file.\n";
                    }
                    $failed = 1;
		}

		push( @params, $name );
	}

	##############################################################
	# Compare first file data data with second file data
	##############################################################
	if ( ( scalar @first_test_data ) != ( scalar @second_test_data ) ) {
		print
		  "[ERROR] Different number of entries in the two files.\n";
          $failed = 1;
	}

	# Iterate through each line of the test results data and compare with the golden data
	foreach $line (@first_test_data) {
		my @first_file_line   = split( /\t/, $line );
		my @second_file_line = split( /\t/, shift @second_test_data );

        my $second_file_arch = trim(@second_file_line[0]);
        my $second_file_circuit = trim(@second_file_line[1]);
        my $first_file_arch = trim(@first_file_line[0]);
        my $first_file_circuit = trim(@first_file_line[1]);

		if (   ( $first_file_circuit ne $first_file_circuit )
			or ( $first_file_arch ne $first_file_arch ) ) {
                    if ($is_golden){
			print "[ERROR] Circuit/Architecture mismatch between golden results ($second_file_arch/$second_file_circuit) and result ($first_file_arch/$first_file_circuit).\n";
                    } else{
                        print "[ERROR] Circuit/Architecture mismatch between first result file ($first_file_arch/$first_file_circuit) and second result fule ($second_file_arch/$second_file_circuit).\n";
                    }
                        $failed = 1;
			return $failed;
		}
		my $circuitarch = "$first_file_arch/$first_file_circuit";

		# Check each parameter where the type determines what to check for
		foreach my $value (@params) {
			my $first_file_index = List::Util::first { $first_test_params[$_] eq $value } 0 .. $#first_test_params;
			my $second_file_index = List::Util::first { $second_test_params[$_] eq $value } 0 .. $#second_test_params;
			my $first_file_value   = trim(@first_file_line[$first_file_index]);
			my $second_file_value = trim(@second_file_line[$second_file_index]);


			if ( $type{$value} eq "Range" or $type{$value} eq "RangeAbs" ) {

				# Check because of division by 0
				if ( $second_file_value == 0 ) {
					if ( $first_file_value != 0 ) {
						if ($is_golden){
                                                    print
                                                        "[Fail] \n $circuitarch $value: golden = $second_file_value generated result = $first_file_value\n";
                                                }else{
                                                    print
                                                        "[Fail] \n $circuitarch $value: first result = $first_file_value second result = $second_file_value\n";
                            }
						$failed = 1;
						return $failed;
					}
				}
				else {
					my $ratio = $first_file_value / $second_file_value;
                    my $abs_diff = abs($first_file_value - $second_file_value);
                    
                    if($verbose) {
                        print "\tParam: $value\n";
                        print "\t\tTest: $first_file_value\n";
                        print "\t\tGolden Value: $second_file_value\n";
                        print "\t\tRatio: $ratio\n";
                    }

					if (   $ratio < $min_threshold{$value}
						or $ratio > $max_threshold{$value} ) {
                        #Beyond relative threshold

                        if (not exists $abs_diff_threshold{$value}
                            or $abs_diff > $abs_diff_threshold{$value}) {
                            #Either no absolute threshold specified, or beyond it
                            if ($is_golden){
                                print
                                    "[Fail] \n $circuitarch $value: golden = $second_file_value generated result = $first_file_value\n";
                            }else{
                                print
                                    "[Fail] \n $circuitarch $value: first result = $first_file_value second result = $second_file_value\n";
                            }
                            $failed = 1;
                            return $failed;
                        }
					}
				}
			} elsif ($type{$value} eq "Equal") {
				if ( $first_file_value ne $second_file_value ) {
					$failed = 1;
                                    if ($is_golden){
                                        print
                                        "[Fail] \n $circuitarch $value: golden = $second_file_value generated result = $first_file_value\n";
                                    }else{
                                        print
                                            "[Fail] \n $circuitarch $value: first result = $first_file_value second result = $second_file_value\n";
                                    }
				}
			} else {
				# If the check type is unknown
                $failed = 1;
                print
                  "[Fail] \n $circuitarch $value: unrecognized check type '$type{$value}' (e.g. Range, RangeAbs, Equal)\n";

			}
		}
	}

	if (!$failed) {
		print "[Pass]\n";
	}
    return $failed;
}

sub trim() {
	my $string = shift;
	$string =~ s/^\s+//;
	$string =~ s/\s+$//;
	return $string;
}

sub expand_user_path {
	my $str = shift;
	$str =~ s/^~\//$ENV{"HOME"}\//;
	return $str;
}

sub get_important_file {
    my $task_path = shift;
    my $vtr_flow_path = shift;
    my $file = shift;
	if ( -e "$task_path/config/$file" ) {
		return "$task_path/config/$file";
	}
	elsif ( -e "$vtr_flow_path/parse/parse_config/$file" ) {
		return "$vtr_flow_path/parse/parse_config/$file";
	}
	elsif ( -e "$vtr_flow_path/parse/qor_config/$file" ) {
	    return "$vtr_flow_path/parse/qor_config/$file";
    }
	elsif ( $file !~ /^\/.*/ ) {
		die "Important file does not exist ($file)";
	}
}

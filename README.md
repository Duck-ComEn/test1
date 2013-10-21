NPT2_Binary
===========
rev.01
Oct/20/2013
-----------

NPT2_binary for debuger
How to compile:
	
	ssh testpc@10.82.134.172
	pass: testpc
		
	cd automation_application_neptune2/testcase
	build.sh //show option building testcase
	
If compile success will be show size of testcase and link of .a file.
If compile error will be show error detail and position failure.

How to debug:
"## use GDB ##"

access to tester with ssh
command:
	>>ps all "list all process is running"
	>>ps all | grep 'PID'
	>>ps all | grep 'testcase'
copy processID of testcase running
	>>gdb -p "processID"
	>>gdb >> b "fileName":"LineNumber"   -->for break 
	>>gdb >> c for continue
	>>gdb >> p "parameter name"
>>remark<< 
	please don't break for long time
	criteria <= 30 minute
end of file
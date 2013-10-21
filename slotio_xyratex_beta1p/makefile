all: 
	@echo "specify target of xcal or optimus (or lower, like lib_xcal or lib_optimus)"

#xcal:	    lib_xcal     samples_xcal
xcal:	    lib_xcal

#optimus:	lib_optimus  samples_optimus
optimus:	lib_optimus
	
	
lib_xcal:
	rm -f makefile.slotio
	ln -s makefile.slotio.xcal makefile.slotio
	./make_lib_xcal.sh
	# (symobic links not supported under samba-mounts, so put minus in front so as not to stop/fail the make)
	-ln -s -f libsi_xcal.a libsi.a
	

samples_xcal:
	cd wrapperSRST_no_threads;./make_xcal.sh
	cd wrapperSRST_with_threads;./make_xcal.sh


lib_optimus:
	rm -f makefile.slotio
	ln -s makefile.slotio.optimus makefile.slotio
	./make_lib_optimus.sh
	# (symobic links not supported under samba-mounts, so put minus in front so as not to stop/fail the make)
	-ln -s -f libsi_optimus.a libsi.a
	

samples_optimus:
	cd wrapperSRST_no_threads;./make_optimus.sh
	cd wrapperSRST_with_threads;./make_optimus.sh
	
clean:
	rm -f *.a
	
man_MANS =			\
	on_ac_power.1		\
	pm-pmu.1

EXTRA_DIST =			\
	pm_on_ac_power.sgml	\
	pm-pmu.sgml		\
	$(man_MANS)

pm-pmu.1: pm-pmu.sgml
	docbook2man $? > $@

pm_on_ac_power.1: pm_on_ac_power.sgml
	docbook2man $? > $@

clean-local :
	rm -f *~
	rm -f *.1
	rm -f manpage.*

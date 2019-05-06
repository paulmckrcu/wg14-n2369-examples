awk '
/^Running / {
	version = $2;
	# print "found version " version; #&&&&
}

/^user/ {
	split($2, ta, "m");
	t = ta[1] * 60 + ta[2];
	timeres[version ":" ++vindex[version]] = t;
	# print "found version time " version, t, vindex[version]; #&&&&
}

END {
	i = 1;
	isdone = 0;
	needcomma="";
	for (v in vindex) {
		printf needcomma "%s", v;
		needcomma=",";
	}
	printf "\n";
	while (!isdone) {
		isdone = 1;
		needcomma="";
		for (v in vindex) {
			printf needcomma "%f", timeres[v ":" i];
			needcomma=",";
			if (i < vindex[v])
				isdone = 0;
		}
		printf "\n";
		i++;
	}
}'

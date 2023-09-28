if [ $EUID -ne 0 ]
then
	echo "uninstall procedure requires root permissions"
	exit 1
fi

rm /usr/bin/medioed
if [ $? -ne 0 ]
then
	echo "cannot uninstall program that was never installed"
	exit 1
fi

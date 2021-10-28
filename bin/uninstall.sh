#!/bin/bash


echo "Uninstall mdqtee"
echo "These files will be deleted:"
echo "/usr/local/bin/mdqtee"
echo "/usr/share/applications/mdqtee.desktop"
echo "/usr/share/pixmaps/mdqtee.png"
echo ""

echo -n "Continue? (y/n): "
 
read ans
 
if [ "$ans" = "y" ] 
then


rm -- "/usr/local/bin/mdqtee"
rm -- "/usr/share/applications/mdqtee.desktop"
rm -- "/usr/share/pixmaps/mdqtee.png"


echo "done"
#read -p "Press [Enter] key to exit..."

else
echo "canceled"
fi


exit 0

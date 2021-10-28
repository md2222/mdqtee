#!/bin/bash


echo "Install mdqtee"
echo "These files will be created:"
echo "/usr/local/bin/mdqtee"
echo "/usr/share/applications/mdqtee.desktop"
echo "/usr/share/pixmaps/mdqtee.png"
echo ""

echo -n "Continue? (y/n): "
 
read ans
 
if [ "$ans" = "y" ] 
then


cp -i -- mdqtee "/usr/local/bin/"
cp -i -- mdqtee.desktop "/usr/share/applications/"
cp -i -- mdqtee.png "/usr/share/pixmaps/"
chmod 755 /usr/local/bin/mdqtee


echo "done"
#read -p "Press [Enter] key to exit..."

else
echo "canceled"
fi


exit 0

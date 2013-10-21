path=$(basename `pwd`)

cp ~/dev/testcase/testcase st/
rm -fR ~/$path/lts/system/etc/*
cp model/* ~/$path/lts/system/etc/
rm -fR ~/$path/lts/system/st/*
cp st/* ~/$path/lts/system/st/

rm -f *.zip

cd st
rm -f st.zip
zip st.zip *
mv st.zip ..
cd ..

cd model
rm -f model.zip
zip model.zip *
mv model.zip ..
cd ..

zip $path.zip model.zip st.zip

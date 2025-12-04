echo "Archiving and compressing civicc..."
COPYFILE_DISABLE=1 tar --exclude='.*' --exclude=coconut/docs --exclude=build --exclude=build-debug --exclude=build-release --exclude=coconut/build -czvf ../civicc.tgz *
echo ======== FINISHED ========
echo "Your archived result is ../civivcc.tgz"
echo ==========================


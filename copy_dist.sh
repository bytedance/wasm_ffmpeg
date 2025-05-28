set -x
# Raname to copy_dist.sh and change target_path

BASE_DIR="$( cd "$(dirname "$0")" >/dev/null 2>&1 ; pwd -P )"

FFMPEG_OUT_PATH=$BASE_DIR/dist
if [[ -z ${FFMPEG_OUT_PATH} ]];then
# debugging .maybe modify in the future
target_path=/tobefill
else
target_path=${FFMPEG_OUT_PATH}
fi 
echo "Copy dist to ${target_path}"

mkdir -p ${target_path}/lib/
mkdir -p ${target_path}/include/
cp -rf $BASE_DIR/dist/include/* ${target_path}/include
cp $BASE_DIR/libffmpeg*.a ${target_path}/lib/
cp $BASE_DIR/libffmpeg*.wasm ${target_path}/lib/


echo "Copy done"
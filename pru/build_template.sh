#!/usr/bin/env bash

cd $(dirname $0)

MAPPING_DIR=mappings
TEMPLATE_DIR=templates
OUTPUT_DIR=generated

PROGRAM_NAME=$1

echo "- Processing template $PROGRAM_NAME..."

for MAPPING_NAME in $(cd "$MAPPING_DIR"; ls *.json|sed s/.json$//); do
	echo "  - Building permutation for mapping $MAPPING_NAME"

	echo "    - Generating mapping headers..."
	node pinmap.js $PROGRAM_NAME --mapping $MAPPING_NAME pru-headers > $OUTPUT_DIR/mapping-$MAPPING_NAME-p.h || exit -1

	echo "    - Applying template..."

	PRU0_FILENAME=$OUTPUT_DIR/$PROGRAM_NAME-$MAPPING_NAME-pru0.p
	echo "#define PRU_NUM 0" > $PRU0_FILENAME
	echo "#define PRU_NUM 0" > $PRU0_FILENAME
	echo "#include \"mapping-$MAPPING_NAME-p.h\"" >> $PRU0_FILENAME
	echo "#include \"../$TEMPLATE_DIR/$PROGRAM_NAME.p\"" >> $PRU0_FILENAME

	PRU1_FILENAME=$OUTPUT_DIR/$PROGRAM_NAME-$MAPPING_NAME-pru1.p
	echo "#define PRU_NUM 1" > $PRU1_FILENAME
	echo "#include \"mapping-$MAPPING_NAME-p.h\"" >> $PRU1_FILENAME
	echo "#include \"../$TEMPLATE_DIR/$PROGRAM_NAME.p\"" >> $PRU1_FILENAME

	echo
done
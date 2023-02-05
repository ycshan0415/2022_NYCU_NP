#!/bin/bash

STUDENT_ID=$1


if [ -z "${STUDENT_ID}" ]; then
    echo "Usage: $0 <student_id>"
    exit 1
fi

mkdir ${STUDENT_ID} && mkdir ${STUDENT_ID}/Part1 && mkdir ${STUDENT_ID}/Part2 \
&& mkdir ${STUDENT_ID}/Part1/server && mkdir ${STUDENT_ID}/Part1/client


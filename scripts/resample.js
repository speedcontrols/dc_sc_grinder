#!/usr/bin/env node

// Resample raw dump file via FIR-decimator

'use strict'

/* eslint-disable no-console */

const fs = require('fs')
const argparse = require('argparse')
const fir_decimate = require('./lib/fir_decimate')

const cli = new argparse.ArgumentParser({
  add_help: true
})

cli.add_argument('-s', '--scale', {
  help: 'ratio',
  type: 'int',
  choices: [2, 3, 4, 8],
  required: true
})

cli.add_argument('file', {
  help: 'File to read',
  nargs: 1
})

const options = cli.parse_args()

const file = fs.readFileSync(options.file[0], { encoding: 'utf8' })
const values = file.replace(/[,]/g, '.').split(/\r?\n/g).map(v => +v)

const resampled = fir_decimate(values, options.scale)

fs.writeFileSync(`${options.file}.resampled`, resampled.join('\n'))

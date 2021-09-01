#!/usr/bin/env node

// Generate spectre of signal, with user-defined resolution (FFT points)
// Takes only first points of raw dump.

'use strict'

/* eslint-disable no-console */

const fs = require('fs')
const argparse = require('argparse')
const fft = require('@signalprocessing/transforms').fft

const cli = new argparse.ArgumentParser({
  add_help: true
})

cli.add_argument('-p', '--points', {
  help: 'number of points',
  type: 'int',
  required: true
})

cli.add_argument('file', {
  help: 'File to read',
  nargs: 1
})

const options = cli.parse_args()

const file = fs.readFileSync(options.file[0], { encoding: 'utf8' })
const values = file.replace(/[,]/g, '.').split(/\r?\n/g).map(v => +v)

const OFFSET = 0
const [out_re, out_im] = fft(values.slice(OFFSET, OFFSET + options.points))

const out = new Array(out_re.length)
for (let i = 0; i < out.length; i++) {
  out[i] = Math.round(Math.sqrt(out_re[i] * out_re[i] + out_im[i] * out_im[i]))
  // out[i] = Math.abs(Math.round(out_re[i]))
}
out[0] = 0 // zero start point, it's not needed and disturbs scale

fs.writeFileSync(`${options.file}.spectre`, out.join('\n'))

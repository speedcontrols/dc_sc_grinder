#!/usr/bin/env node

'use strict'

const IGNORE_BAND_HZ = 500
const MIN_PEAK = 168014 / 2

/* eslint-disable no-console */

const fs = require('fs')
const argparse = require('argparse')
const fft = require('@signalprocessing/transforms').fft

const cli = new argparse.ArgumentParser({
  add_help: true
})

cli.add_argument('-s', '--sampling', {
  help: 'sampling frequency',
  type: 'int',
  default: 16384
})

cli.add_argument('file', {
  help: 'File to read',
  nargs: 1
})

const options = cli.parse_args()

const file = fs.readFileSync(options.file[0], { encoding: 'utf8' })
const values = file.replace(/[,]/g, '.').split(/\r?\n/g).map(v => +v)

const result = []

for (let pos = 0; ;) {
  const fft_points = 1024

  if (pos + fft_points > values.length) break

  const [out_re, out_im] = fft(values.slice(pos, pos + fft_points))

  const out = new Array(out_re.length)
  for (let i = 0; i < out.length; i++) {
    out[i] = Math.round(Math.sqrt(out_re[i] * out_re[i] + out_im[i] * out_im[i]))
    // out[i] = Math.abs(Math.round(out_re[i]))
  }

  const resolution = options.sampling / fft_points
  const skip = Math.ceil(IGNORE_BAND_HZ / resolution) + 1

  let idx = skip
  let max = out[skip]

  for (let i = skip; i < (fft_points / 2); i++) {
    if (out[i] > max) {
      idx = i
      max = out[i]
    }
  }

  let freq = idx * resolution

  if (max < MIN_PEAK) freq = 0;

  result.push(`freq = ${freq}, peak = ${max}, rpm = ${freq / 8 * 60}`)

  pos += fft_points
}

fs.writeFileSync(`${options.file}.detect`, result.join('\n'))

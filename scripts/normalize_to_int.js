#!/usr/bin/env node

'use strict'

// Normalize dumps to int range 0-65535

const SRC_MAX_VALUE = 4.096

/* eslint-disable no-console */

const fs = require('fs')
const argparse = require('argparse')

const cli = new argparse.ArgumentParser({
  add_help: true
})

cli.add_argument('file', {
  help: 'File to read',
  nargs: 1
})

const options = cli.parse_args()

const file = fs.readFileSync(options.file[0], { encoding: 'utf8' })
const values = file
  .trim()
  .replace(/[,]/g, '.')
  .split(/\r?\n/g)
  .map(v => parseFloat(v))

const out = values
  .map(v => Math.round(v * 65536 / SRC_MAX_VALUE))
  .map(v => v > 65535 ? 65535 : v)
  .map(v => v < 0 ? 0 : v)

fs.writeFileSync(`${options.file}.normalized`, out.join('\n'))

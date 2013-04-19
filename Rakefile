# -*- mode: ruby; coding: utf-8 -*-
require "rake/clean"
require "rdoc/task"

THIS_FILE = File.expand_path(__FILE__)
THIS_DIR = File.dirname(THIS_FILE)

CLEAN.include "ext/*.o", "ext/Makefile", "ext/mkmf.log"
CLOBBER.include "ext/*.so"

task :default => :compile

desc "Compiles the library."
task :compile do
  cd "ext" do
    ruby "extconf.rb"
    sh "make"
  end
end

task :console do
  ARGV.clear # IRB doesn’t like this
  require "irb"
  require "#{THIS_DIR}/ext/alpm"
  IRB.start
end

RDoc::Task.new do |r|
  r.generator = "emerald"
  r.rdoc_files.include("ext/**/*.c", "**/**/*.rdoc", "COPYING")
  r.title = "Alpm-Ruby RDocs"
  r.main = "README.rdoc"
  r.rdoc_dir = "doc"
end

require 'fileutils'
require 'zlib'
require 'rubygems/package'

module TarGzip

  def self.pack(tar_gz_file, base_dir, entries)
    self.write_tar_gz(tar_gz_file) do |tar|
      list_files(base_dir, entries).each do |file|
        path = File.join(base_dir, file)
        stat = File.stat(path)

        tar.add_file_simple(file, stat.mode, stat.size) do |tar_file|
          open(path, 'rb') do |file|
            tar_file.write(file.read(16384)) until file.eof?
          end
        end
      end
    end
  end

  def self.unpack(tar_gz_file, base_dir)
    self.read_tar_gz(tar_gz_file) do |entry|
      path = File.join(base_dir, entry.full_name)

      FileUtils.rm_rf(path)

      FileUtils.mkdir_p(File.dirname(path))

      open(path, 'wb', entry.header.mode) do |out|
        out.write(entry.read(16384)) until entry.eof?
      end
    end
  end

  private

  def self.list_files(base_dir, entries)
    files = []
    entries.each do |entry|
      path = File.join(base_dir, entry)
      if File.directory?(path)
        Dir.glob(File.join(path, '**/*'), File::FNM_DOTMATCH) do |f|
          files << Pathname.new(f).relative_path_from(Pathname.new(base_dir)).to_s if File.file?(f)
        end
      else
        files << entry
      end
    end
    files
  end

  def self.write_tar_gz(tar_gz_file)
    Zlib::GzipWriter.open(tar_gz_file) do |gz|
      Gem::Package::TarWriter.new(gz) do |tar|
        yield tar
      end
    end
  end

  def self.read_tar_gz(tar_gz_file)
    Zlib::GzipReader.open(tar_gz_file) do |gz|
      Gem::Package::TarReader.new(gz).each do |entry|
        yield entry
      end
    end
  end
end
=begin
require 'pathname'

base_dir = 'C:/work'
files = []
Dir.glob(File.join(base_dir, 'rho/**/*'), File::FNM_DOTMATCH) do |f|
  files << Pathname.new(f).relative_path_from(Pathname.new(base_dir)).to_s if File.file?(f)
end
TarGz.pack('Z:/shared/tmp/ugu.tar.gz', base_dir, files)
TarGz.unpack('Z:/shared/tmp/ugu.tar.gz', 'Z:/shared/tmp/ugu')
=end

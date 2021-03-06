#include "output_writer.h"

namespace tomahawk{
namespace io{

OutputWriter::OutputWriter(void) :
	owns_pointers(true),
	writing_sorted_(false),
	writing_sorted_partial_(false),
	n_entries(0),
	n_progress_count(0),
	n_blocks(0),
	l_flush_limit(2000000),
	l_largest_uncompressed(0),
	stream(nullptr),
	buffer(this->l_flush_limit*2),
	spin_lock(new spin_lock_type),
	index_(new index_type),
	footer_(new footer_type)
{

}

OutputWriter::OutputWriter(std::string input_file) :
	owns_pointers(true),
	writing_sorted_(false),
	writing_sorted_partial_(false),
	n_entries(0),
	n_progress_count(0),
	n_blocks(0),
	l_flush_limit(2000000),
	l_largest_uncompressed(0),
	stream(new std::ofstream(input_file, std::ios::binary | std::ios::out)),
	buffer(this->l_flush_limit*2),
	spin_lock(new spin_lock_type),
	index_(new index_type),
	footer_(new footer_type)
{

}

OutputWriter::OutputWriter(const self_type& other) :
	owns_pointers(false),
	writing_sorted_(other.writing_sorted_),
	writing_sorted_partial_(other.writing_sorted_partial_),
	n_entries(other.n_entries),
	n_progress_count(other.n_progress_count),
	n_blocks(other.n_blocks),
	l_flush_limit(other.l_flush_limit),
	l_largest_uncompressed(0),
	stream(other.stream),
	buffer(other.buffer.capacity()),
	spin_lock(other.spin_lock),
	index_(other.index_),
	footer_(other.footer_)
{

}

OutputWriter::~OutputWriter(void){
	if(this->owns_pointers){
		this->stream->flush();
		this->stream->close();
		delete this->stream;
		delete this->spin_lock;
		delete this->index_;
		delete this->footer_;
	}
}

bool OutputWriter::open(const std::string& output_file){
	if(output_file.size() == 0)
		return false;

	this->CheckOutputNames(output_file);
	this->filename = output_file;

	this->stream = new std::ofstream(this->basePath + this->baseName + '.' + tomahawk::constants::OUTPUT_LD_SUFFIX, std::ios::binary | std::ios::out);
	if(this->stream->good() == false){
		std::cerr << "Failed to open: " << output_file << std::endl;
		return false;
	}

	return true;
}

int OutputWriter::writeHeaders(twk_header_type& twk_header){
	const std::string command = "##tomahawk_calcCommand=" + helpers::program_string();
	twk_header.getLiterals() += command;
	// Set file type to TWO
	twk_header.magic_.file_type = 1;

	return(twk_header.write(*this->stream));
}

void OutputWriter::writeFinal(void){
	this->footer_->l_largest_uncompressed = this->l_largest_uncompressed;
	this->footer_->offset_end_of_data = this->stream->tellp();
	this->index_->setSorted(this->isSorted());
	this->index_->setPartialSorted(this->isPartialSorted());

	this->stream->flush();
	*this->stream << *this->index_;
	*this->stream << *this->footer_;
	this->stream->flush();
}

void OutputWriter::flush(void){
	if(this->buffer.size() > 0){
		if(!this->compressor.Deflate(this->buffer)){
			std::cerr << helpers::timestamp("ERROR","TGZF") << "Failed deflate DATA..." << std::endl;
			exit(1);
		}

		if(this->buffer.size() > l_largest_uncompressed)
			this->l_largest_uncompressed = this->buffer.size();

		this->spin_lock->lock();
		this->index_entry.byte_offset = (U64)this->stream->tellp();
		this->index_entry.uncompressed_size = this->buffer.size();
		this->stream->write(this->compressor.buffer.data(), this->compressor.buffer.size());
		this->index_entry.byte_offset_end = (U64)this->stream->tellp();
		this->index_entry.n_variants = this->buffer.size() / sizeof(entry_type);
		//*this->stream << this->index_entry;
		this->index_->getContainer() += this->index_entry;
		//std::cerr << this->index_entry.byte_offset_from << "->" << this->index_entry.byte_offset_to << " for " << this->index_entry.n_entries << " of " << this->index_entry.uncompressed_size << std::endl;
		++this->n_blocks;

		this->spin_lock->unlock();

		this->buffer.reset();
		this->compressor.Clear();
		this->index_entry.reset();
	}
}

void OutputWriter::operator<<(const container_type& container){
	for(size_type i = 0; i < container.size(); ++i)
		this->buffer << container[i];

	this->n_entries += buffer.size() / sizeof(entry_type);
	*this << this->buffer;
}

void OutputWriter::operator<<(buffer_type& buffer){
	if(buffer.size() > 0){
		if(!this->compressor.Deflate(buffer)){
			std::cerr << helpers::timestamp("ERROR","TGZF") << "Failed deflate DATA..." << std::endl;
			exit(1);
		}

		if(buffer.size() > l_largest_uncompressed)
			this->l_largest_uncompressed = buffer.size();

		// Lock
		this->spin_lock->lock();

		this->index_entry.byte_offset       = (U64)this->stream->tellp();
		this->index_entry.uncompressed_size = buffer.size();
		this->stream->write(this->compressor.buffer.data(), this->compressor.buffer.size());
		this->index_entry.byte_offset_end   = (U64)this->stream->tellp();
		this->index_entry.n_variants        = buffer.size() / sizeof(entry_type);
		this->index_->getContainer()       += this->index_entry;
		++this->n_blocks;

		// Unlock
		this->spin_lock->unlock();

		buffer.reset();
		this->compressor.Clear();
		this->index_entry.reset();
	}
}

void OutputWriter::writePrecompressedBlock(buffer_type& buffer, const U64& uncompressed_size){
	if(buffer.size() == 0) return;

	assert(uncompressed_size % sizeof(entry_type) == 0);

	if(uncompressed_size > l_largest_uncompressed)
		this->l_largest_uncompressed = uncompressed_size;

	// Lock
	this->spin_lock->lock();

	this->index_entry.byte_offset       = (U64)this->stream->tellp();
	this->index_entry.uncompressed_size = uncompressed_size;
	this->stream->write(buffer.data(), buffer.size());
	this->index_entry.byte_offset_end   = (U64)this->stream->tellp();
	this->index_entry.n_variants        = uncompressed_size / sizeof(entry_type);
	this->index_->getContainer()       += this->index_entry;
	++this->n_blocks;

	// Unlock
	this->spin_lock->unlock();

	buffer.reset();
	this->index_entry.reset();
}

void OutputWriter::CheckOutputNames(const std::string& input){
	std::vector<std::string> paths = helpers::filePathBaseExtension(input);
	this->basePath = paths[0];
	if(this->basePath.size() > 0)
		this->basePath += '/';

	if(paths[3].size() == tomahawk::constants::OUTPUT_LD_SUFFIX.size() && strncasecmp(&paths[3][0], &tomahawk::constants::OUTPUT_LD_SUFFIX[0], tomahawk::constants::OUTPUT_LD_SUFFIX.size()) == 0)
		this->baseName = paths[2];
	else this->baseName = paths[1];
}

void OutputWriter::Add(const MetaEntry& meta_a, const MetaEntry& meta_b, const header_entry_type& header_a, const header_entry_type& header_b, const entry_support_type& helper){
	const U32 writePosA = meta_a.position << 2 | meta_a.all_phased << 1 | meta_a.has_missing;
	const U32 writePosB = meta_b.position << 2 | meta_b.all_phased << 1 | meta_b.has_missing;
	this->buffer += helper.controller;
	this->buffer += header_a.contigID;
	this->buffer += writePosA;
	this->buffer += header_b.contigID;
	this->buffer += writePosB;
	this->buffer << helper;

	// Todo: toggleable
	// Add reverse
	this->buffer += helper.controller;
	this->buffer += header_b.contigID;
	this->buffer += writePosB;
	this->buffer += header_a.contigID;
	this->buffer += writePosA;
	this->buffer << helper;

	this->n_entries += 2;
	this->n_progress_count += 2;
	this->index_entry.n_variants += 2;

	if(this->buffer.size() > this->l_flush_limit)
		this->flush();
}

}
}

#ifndef ALGORITHM_GENOTYPE_BITPACKER_H_
#define ALGORITHM_GENOTYPE_BITPACKER_H_

namespace tomahawk{
namespace algorithm{

#define PACKER_BITS_BYTE (sizeof(BYTE)*8)

/**<
 * This class packs run-length encoded genotypes
 * into a 2-bit encoding system used in vectorized
 * functions throughout Tomahawk
 */
class GenotypeBitPacker{
public:
	GenotypeBitPacker(BYTE* destination, const BYTE stepsize) :
		stepSize(stepsize),
		steps(PACKER_BITS_BYTE/stepsize),
		mask((1 << stepsize) - 1),
		currentOffset(this->steps),
		destination_pointer(0),
		destination(destination)
	{}

	~GenotypeBitPacker(){}

	void add(const BYTE& value, const U32 repeat_times){
		if(this->currentOffset == 0){
			this->currentOffset = this->steps;
			++this->destination_pointer;
		}

		U32 remainder = repeat_times;
		if(this->currentOffset != this->steps){
			// If the number of repeats exceeds what is available
			// keep adding until hitting the end
			if(repeat_times >= this->currentOffset){
				for(S32 i = this->currentOffset; i > 0; --i)
					this->destination[this->destination_pointer] ^= (value & this->mask) << ((i - 1) * this->stepSize);

				remainder -= this->currentOffset;
				this->currentOffset = this->steps;
				++this->destination_pointer;
			} else {
				for(S32 i = this->currentOffset; i > (this->currentOffset - repeat_times); --i)
					this->destination[this->destination_pointer] ^= (value & this->mask) << ((i - 1) * this->stepSize);


				remainder -= repeat_times;
				this->currentOffset -= repeat_times;
			}

			if(remainder == 0)
				return;
		}

		// Number of complete additions
		const U32 remainder_times_balanced = remainder/this->steps;
		if(remainder_times_balanced > 0){
			BYTE copy_me = 0;
			for(S32 i = this->steps; i > 0; --i)
				copy_me ^= (value & this->mask) << ((i-1)*this->stepSize);

			// Memcpy fails alignment and produces nonsense! Use a loop
			for(U32 i = 0; i < remainder_times_balanced; ++i){
				this->destination[this->destination_pointer] = copy_me;
				++this->destination_pointer;
			}
			remainder -= remainder_times_balanced*this->steps;
		}

		for(U32 i = 0; i < remainder; ++i){
			this->destination[this->destination_pointer] ^= (value & this->mask) << ((this->currentOffset - 1) * this->stepSize);
			--this->currentOffset;
		}
	}

	inline U32 size(void) const{ return(this->destination_pointer*this->steps + (this->steps - this->currentOffset)); }
	inline void setDestination(BYTE* dest){ this->destination = dest; }
	inline const BYTE& operator[](const U32& p) const{ return(this->destination[p]); }

private:
	const BYTE stepSize;		// step size (in bits)
	const BYTE steps;			// number of steps in byte
	const BYTE mask;			// insert mask
	S32   currentOffset;
	U32   destination_pointer;
	BYTE* destination;
};

}
}

#endif /* ALGORITHM_GENOTYPE_BITPACKER_H_ */

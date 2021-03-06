#ifndef TOMAHAWK_TOMAHAWKOUTPUTLD_H_
#define TOMAHAWK_TOMAHAWKOUTPUTLD_H_

#include "io/basic_buffer.h"

namespace tomahawk {
namespace support{

struct OutputEntrySupport{
private:
	typedef OutputEntrySupport self_type;
	typedef io::BasicBuffer    buffer_type;

public:
	OutputEntrySupport();
	~OutputEntrySupport();

	inline void resetPhased(void){
		this->controller      = 0;
		this->alleleCounts[0] = 0;
		this->alleleCounts[1] = 0;
		this->alleleCounts[4] = 0;
		this->alleleCounts[5] = 0;
		this->chiSqModel      = 0;
		this->R2 = 0;
		// All other values can legally overflow
		// They are not used
	}

	inline void resetUnphased(void){
		this->controller       = 0;
		this->alleleCounts[0]  = 0;
		this->alleleCounts[1]  = 0;
		this->alleleCounts[4]  = 0;
		this->alleleCounts[5]  = 0;
		this->alleleCounts[16] = 0;
		this->alleleCounts[17] = 0;
		this->alleleCounts[20] = 0;
		this->alleleCounts[21] = 0;
		this->alleleCounts[64] = 0;
		this->alleleCounts[65] = 0;
		this->alleleCounts[68] = 0;
		this->alleleCounts[69] = 0;
		this->alleleCounts[80] = 0;
		this->alleleCounts[81] = 0;
		this->alleleCounts[84] = 0;
		this->alleleCounts[85] = 0;
		// All other values can legally overflow
		// They are not used
	}

	inline double& operator[](const U32& p){ return(this->alleleCounts[p]); }
	inline const double& operator[](const U32& p) const{ return(this->alleleCounts[p]); }
	void operator=(const OutputEntrySupport& other);
	void printUnphasedCounts(void) const;
	void printPhasedCounts(void) const;

	friend io::BasicBuffer& operator<<(buffer_type& os, const self_type& entry){
		// Notice that CONTROLLER is written separately
		os += entry.alleleCounts[0];
		os += entry.alleleCounts[1];
		os += entry.alleleCounts[4];
		os += entry.alleleCounts[5];
		os += entry.D;
		os += entry.Dprime;
		os += entry.R;
		os += entry.R2;
		os += entry.P;
		os += entry.chiSqFisher;
		os += entry.chiSqModel;
		return os;
	}

	friend std::ostream& operator<<(std::ostream& os, const self_type& entry){
		os << entry.alleleCounts[0] << '\t' << entry.alleleCounts[1] << '\t' << entry.alleleCounts[4] << '\t' << entry.alleleCounts[5] << '\t'
		   << entry.D << '\t' << entry.Dprime << '\t' << entry.R << '\t' << entry.R2 << '\t' << entry.P << '\t' << entry.chiSqFisher;
		return os;
	}

	inline void setUsedPhasedMath(const bool yes = true)   { this->controller |= yes << 0;  } // 1
	inline void setSameContig(const bool yes = true)       { this->controller |= yes << 1;  } // 2
	inline void setLongRange(const bool yes = true)        { this->controller |= yes << 2;  } // 4
	inline void setCompleteLD(const bool yes = true)       { this->controller |= yes << 3;  } // 8
	inline void setPerfectLD(const bool yes = true)        { this->controller |= yes << 4;  } // 16
	inline void setMultipleRoots(const bool yes = true)    { this->controller |= yes << 5;  } // 32
	inline void setFastMode(const bool yes = true)         { this->controller |= yes << 6;  } // 64
	inline void setSampled(const bool yes = true)          { this->controller |= yes << 7;  } // 128
	inline void setHasMissingValuesA(const bool yes = true){ this->controller |= yes << 8;  } // 256
	inline void setHasMissingValuesB(const bool yes = true){ this->controller |= yes << 9;  } // 512
	inline void setLowAFA(const bool yes = true)           { this->controller |= yes << 10; } // 1024
	inline void setLowAFB(const bool yes = true)           { this->controller |= yes << 11; } // 2048
	inline void setFailedHWEA(const bool yes = true)       { this->controller |= yes << 12; } // 4096
	inline void setFailedHWEB(const bool yes = true)       { this->controller |= yes << 13; } // 8192

	inline const double getTotalAltHaplotypeCount(void) const{
		// Find largest
		const double* max = &this->alleleCounts[0];
		if(this->alleleCounts[1] > *max) max = &this->alleleCounts[1];
		if(this->alleleCounts[4] > *max) max = &this->alleleCounts[4];
		if(this->alleleCounts[5] > *max) max = &this->alleleCounts[5];

		// Count number of non-major allele combinations
		double max2 = 0;
		if(&this->alleleCounts[0] != max) max2 += this->alleleCounts[0];
		if(&this->alleleCounts[1] != max) max2 += this->alleleCounts[1];
		if(&this->alleleCounts[4] != max) max2 += this->alleleCounts[4];
		if(&this->alleleCounts[5] != max) max2 += this->alleleCounts[5];

		return(max2);
	}

public:
	U16    controller;        // FLAG byte
	float  R, R2;             // Correlation coefficients
	float  D, Dprime, Dmax;   // D values
	double P;                 // Fisher or Chi-Squared P value for 2x2 contingency table
	double chiSqModel;        // Chi-Squared critical value for 3x3 contingency table
	double chiSqFisher;       // Chi-Squared critical value for 2x2 contingency table
	double totalHaplotypeCounts; // Total number of alleles

	// Counters
	double alleleCounts[171];
	double haplotypeCounts[4];
};

} /* namespace Support */
} /* namespace Tomahawk */

#endif /* TOMAHAWK_TOMAHAWKOUTPUTLD_H_ */

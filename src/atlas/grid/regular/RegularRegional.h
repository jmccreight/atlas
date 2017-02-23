#ifndef atlas_grid_regular_RegularRegional_h
#define atlas_grid_regular_RegularRegional_h

#include "atlas/grid/regular/Regular.h"
#include "atlas/grid/spacing/LinearSpacing.h"

namespace atlas {
namespace grid {
namespace regular {

class RegularRegional: public Regular {

public:

    static std::string grid_type_str();

    static std::string className();

    virtual std::string shortName() const;
    virtual std::string gridType() const { return "regular_regional"; }

    RegularRegional(const util::Config& params);
    RegularRegional();

private:

    void setup(const util::Config& params);
    std::string shortName();

};


}  // namespace regular
}  // namespace grid
}  // namespace atlas


#endif

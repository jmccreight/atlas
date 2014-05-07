

#include <sstream>
#include <stdexcept>

#include "atlas/Mesh.hpp"
#include "atlas/FunctionSpace.hpp"

namespace atlas {

Mesh::~Mesh() 
{ 
//  std::cout << "Mesh Destructor " << std::endl;

  index_.clear();
  for( size_t f=0; f<function_spaces_.size(); ++f )
    if( function_spaces_[f] ) delete(function_spaces_[f]);
  function_spaces_.clear();
}

bool Mesh::has_function_space(const std::string &name)
{
    return (index_.find(name) != index_.end());
}

FunctionSpace& Mesh::add_function_space( FunctionSpace* function_space )
{
  if (index_.count(function_space->name()) )
  {
    throw std::runtime_error("Functionspace "+function_space->name()+" already added");
  }
  index_[function_space->name()] = function_spaces_.size();
  function_space->set_index( index_[function_space->name()] );
  function_spaces_.push_back( function_space );
  return *function_space;
}

FunctionSpace& Mesh::function_space(const std::string& name)
{
  try {
    return *function_spaces_[ index_.at(name) ];
  }
  catch( std::out_of_range& e ) {
    std::stringstream msg;
    msg << "Could not find function_space \"" << name << "\" in mesh";
    throw std::out_of_range(msg.str());
  }
}

FunctionSpace& Mesh::function_space(int idx)
{
  return *function_spaces_[ idx ];
}


// ------------------------------------------------------------------
// C wrapper interfaces to C++ routines

Mesh* atlas__Mesh__new () { 
  return new Mesh(); 
}

void atlas__Mesh__delete (Mesh* This) {
  delete This;
}

void atlas__Mesh__add_function_space (Mesh* This, FunctionSpace* function_space) {
  This->add_function_space(function_space);
}

FunctionSpace* atlas__Mesh__function_space (Mesh* This, char* name) {
  return &This->function_space( std::string(name) );
}
// ------------------------------------------------------------------


} // namespace atlas


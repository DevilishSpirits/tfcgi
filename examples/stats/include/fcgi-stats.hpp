#include <fcgi.hpp>
namespace fcgi {
	namespace examples {
		namespace stats {
			void dump_to_json(std::ostream& cout);
			void dump_to_json(std::ostream& cout, fcgi::GlobalStats &stats);
			int respond_json(fcgi::Request &request);
		}
	}
}

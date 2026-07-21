# Profil callgrind (deterministyczny) — processRows po K1

- data: 2026-07-21T09:39:10+02:00
- narzedzie: valgrind callgrind, --toggle-collect='*processRows*', -m 200 (rec205-detect)
- zebrano: 312,271,266 instrukcji w processRows (~1.56M/interwal)
- WSL2: instrukcje sa DETERMINISTYCZNE (niezalezne od szumu czasu)

## Inclusive (koszt fazy z podfunkcjami, % processRows)
```
157,246,819 (50.36%)  ???:dataModel::constructInputPayload(std::__cxx11::basic_string<char, std::char_traits<char>, st
126,643,681 (40.56%)  ???:streamInstance::constructOutputPayload(std::__cxx11::list<field, std::allocator<field> > con
 98,223,809 (31.45%)  ???:streamInstance::constructAgsePayload(int, int, std::__cxx11::basic_string<char, std::char_tr
 69,247,687 (22.18%)  ???:rdb::payload::setItem(int, std::optional<std::any>) 
 67,564,205 (21.64%)  ???:expressionEvaluator::eval(std::__cxx11::list<token, std::allocator<token> > const&, rdb::pay
 60,538,351 (19.39%)  ???:rdb::storage::revRead(unsigned long, unsigned char*) 
 58,775,531 (18.82%)  ???:rdb::storage::read(unsigned long, unsigned char*) 
 49,785,346 (15.94%)  ???:cast<std::any>::operator()(std::any const&, rdb::descFld) 
 34,342,614 (11.00%)  ???:streamInstance::reduceFieldsToPayload(command_id, std::__cxx11::basic_string<char, std::char
 26,837,500 ( 8.59%)  ???:rdb::memoryFile::read(unsigned char*, std::vector<bool, std::allocator<bool> >&, unsigned lo
 26,571,078 ( 8.51%)  ???:void visit_descFld<int, std::any>(std::any const&, std::any&) 
 26,519,684 ( 8.49%)  ???:rdb::(anonymous namespace)::resolveFieldIndexOrAbort(rdb::Descriptor const&, int, char const
 22,282,795 ( 7.14%)  ???:rdb::storage::write(unsigned long) 
 22,191,343 ( 7.11%)  ???:rdb::payload::getItem(int) const 
 21,605,432 ( 6.92%)  ???:std::any::_Manager_internal<int>::_S_manage(std::any::_Op, std::any const*, std::any::_Arg*)
 20,237,510 ( 6.48%)  ./string/../sysdeps/x86_64/multiarch/memcmp-avx2-movbe.S:__memcmp_avx2_movbe
 18,511,976 ( 5.93%)  ???:std::type_info::operator==(std::type_info const&) const [clone .isra.0] 
 16,947,667 ( 5.43%)  ???:std::map<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, st
 14,471,916 ( 4.63%)  ???:rdb::metaData::flushCurrentEntry() 
 14,352,404 ( 4.60%)  ???:any_to_variant_cast[abi:cxx11](std::any) 
 12,956,540 ( 4.15%)  ???:rdb::metaData::nullBitsetFor(unsigned long) const 
 12,536,824 ( 4.01%)  ???:rdb::MetaIndexStore::overwriteLast(rdb::IndexRecord const&) 
```

## Self (self-koszt funkcji, % processRows)
```
21,605,432 ( 6.92%)  ???:std::any::_Manager_internal<int>::_S_manage(std::any::_Op, std::any const*, std::any::_Arg*) 
20,237,510 ( 6.48%)  ./string/../sysdeps/x86_64/multiarch/memcmp-avx2-movbe.S:__memcmp_avx2_movbe
17,602,500 ( 5.64%)  ???:rdb::(anonymous namespace)::resolveFieldIndexOrAbort(rdb::Descriptor const&, int, char const*
13,929,049 ( 4.46%)  ???:rdb::payload::setItem(int, std::optional<std::any>) 
12,201,685 ( 3.91%)  ???:expressionEvaluator::eval(std::__cxx11::list<token, std::allocator<token> > const&, rdb::payl
10,203,750 ( 3.27%)  ???:cast<std::any>::operator()(std::any const&, rdb::descFld) 
 9,603,440 ( 3.08%)  ./string/../sysdeps/x86_64/multiarch/strcmp-avx2.S:__strcmp_avx2
 9,061,776 ( 2.90%)  ???:rdb::payload::getItem(int) const 
 8,995,106 ( 2.88%)  ???:std::map<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std
 8,908,536 ( 2.85%)  ???:std::type_info::operator==(std::type_info const&) const [clone .isra.0] 
 7,615,793 ( 2.44%)  ???:std::__detail::__variant::_Copy_ctor_base<false, unsigned char, int, unsigned int, boost::rat
 7,388,340 ( 2.37%)  ./string/../sysdeps/x86_64/multiarch/memmove-vec-unaligned-erms.S:__memcpy_avx_unaligned_erms
 7,216,836 ( 2.31%)  ???:void visit_descFld<int, std::any>(std::any const&, std::any&) 
 6,688,950 ( 2.14%)  ???:rdb::Descriptor::flatIndexToDescriptorPosition(int) const 
 6,271,520 ( 2.01%)  ???:rdb::memoryFile::read(unsigned char*, std::vector<bool, std::allocator<bool> >&, unsigned lon
 5,798,099 ( 1.86%)  ???:void std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >::_M_cons
 5,116,091 ( 1.64%)  ???:token::getStr_[abi:cxx11]() const 
 5,074,499 ( 1.63%)  ???:std::vector<bool, std::allocator<bool> >::vector(std::vector<bool, std::allocator<bool> > con
 4,680,940 ( 1.50%)  ???:rdb::memoryFile::count() 
 4,470,564 ( 1.43%)  ???:decltype(auto) std::__do_visit<void, std::__detail::__variant::_Variant_storage<false, unsign
 4,453,522 ( 1.43%)  ???:streamInstance::constructOutputPayload(std::__cxx11::list<field, std::allocator<field> > cons
 4,400,644 ( 1.41%)  ???:rdb::payload::setNullBitset(std::vector<bool, std::allocator<bool> > const&) 
 4,231,800 ( 1.36%)  ???:rdb::Descriptor::byteOffsetAtFlatIndex(int) const 
 3,647,670 ( 1.17%)  ???:std::map<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std
 3,519,085 ( 1.13%)  ???:void* std::__any_caster<int>(std::any const*) 
```

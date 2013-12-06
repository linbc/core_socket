/*
 * ByteArray.h
 *
 *  Created on: 2013-9-30
 *      Author: linbc
 */

#ifndef BYTEARRAY_H_
#define BYTEARRAY_H_

#include <core_obj-internal.h>

namespace core_obj {

class ByteArray {
public:
	typedef std::vector<char> BUFFERS;

	ByteArray():position_(0){};
	virtual ~ByteArray(){};

	inline char *cur_data(){
		return bytes_.data() + position_;
	}

	inline int length(){
		return bytes_.size();		
	}

	inline void length(int s){
		bytes_.resize(s);
		if(position_ >= s)
			position_ = s;
	}

	inline int position(){
		return position_;
	}

	inline bool position(int v){
		if(v < length()){
			position_ = v;
			return true;
		}
		return false;
	}
	
	inline int bytesAvailable(){
		return (int)bytes_.size()-position_;
	}

	inline int8_t readByte(){
		return readT<int8_t>();
	}

	inline double readDouble(){
		return readT<double>();
	}

	inline float readFloat(){
		return readT<float>();
	}

	inline int32_t readInt(){
		return readT<int32_t>();
	}

	inline int16_t readShort(){
		return readT<int16_t>();
	}

	inline uint8_t readUnsignedByte(){
		return readT<uint8_t>();
	}

	inline uint32_t readUnsignedInt(){
		return readT<uint32_t>();
	}

	inline uint16_t readUnsignedShort(){
		return readT<uint16_t>();
	}

	string readUTF(){
		int len = readT<uint16_t>();
		string val;
		val.resize(len-1);
		BUFFERS::iterator it = bytes_.begin();
		std::copy(it+position_,it+position_+len-1,val.begin());
		position_+= len;
		return val;
	}

	inline ByteArray& writeDouble(double val){
		return writeT<double>(val);
	}

	inline ByteArray& writeFloat(float val){
		return writeT<float>(val);
	}

	inline ByteArray& writeInt(int32_t val){
		return writeT<int32_t>(val);
	}

	inline ByteArray& writeShort(short val){
		return writeT<int16_t>(val);
	}
	
	inline ByteArray& writeByte(int8_t val){
		return writeT<int8_t>(val);
	}

	inline ByteArray& writeUnsignedInt(uint32_t val){
		return writeT<uint32_t>(val);
	}

	inline ByteArray& writeUTF(string str){
		size_t len = (uint16_t)str.size()+1;
		writeT<uint16_t>(len);
		
		if(bytes_.size()<position_+len)
			bytes_.resize(position_+len);
		std::copy(str.begin(),str.end(),bytes_.begin()+position_);
		position_+=len;
		return *this;
	}

	//将指定字节数组 bytes（起始偏移量为 offset，从零开始的索引）中包含 length 个字节的字节序列写入字节流。 
	inline ByteArray& writeBytes(const ByteArray& bytes,int offset = 0,int len = 0)
	{
		throw std::exception("writeBytes");
		return *this;
	}

	inline ByteArray& writeBytes(uint8_t* pos,int len)
	{
		if((int)bytes_.size()-position_ < len)
			bytes_.resize(position_+len);
		memcpy(bytes_.data()+position_,pos,len);
		return *this;
	}

	template<typename T> 
	inline T readT(){
		T *p = (T*)(&bytes_[position_]);
		position_ += sizeof(T);
		return *p;		
	}

	template<typename T> 
	inline ByteArray& operator>>(T &t)
	{
		memcpy(&t,&bytes_[position_],sizeof(t));
		position_ += sizeof(T);
		return *this;
	}

	inline ByteArray& operator>>(string &str)
	{
		str = readUTF();
		return *this;
	}

	inline ByteArray& operator>>(const char* & s)
	{
		uint16_t len = readUnsignedShort();
		if(position_+len > (int)bytes_.size())
			throw std::exception("out of bytes");
		s = (char*)&(bytes_[position_]);
		position_ += len;
		return *this;
	}	

	template<typename T> 
	inline ByteArray& writeT(T val)
	{
		if(bytes_.size()<position_+sizeof(T))
			bytes_.resize(position_+sizeof(T));
		T *p = (T*)(&bytes_[position_]);
		position_ += sizeof(T);
		*p = val;
		return *this;
	}

	template<typename T> 
	inline ByteArray& operator<< (T t)
	{
		return writeT<T>(t);
	}

	inline ByteArray& operator<< (string& str)
	{
		return writeUTF(str);
	}

	inline ByteArray& operator<< (string str)
	{
		return writeUTF(str);
	}

	inline ByteArray& operator<< (const char* s)
	{
		return writeUTF(s);
	}

private:
	int position_;
	BUFFERS bytes_;
};


//vector
template<typename T>
inline ByteArray &operator<<(ByteArray& dst,const std::vector<T>& value)
{
	uint16_t _size = (uint16_t)value.size();
	dst<<_size;
	for (typename std::vector<T>::const_iterator iter = value.begin();iter!=value.end();++iter)
	{
		dst<< *iter;
	}
	return dst;
}

template<typename T>
inline ByteArray &operator>>(ByteArray& src,std::vector<T>& value)
{
	uint16_t __size;
	src>>__size;
	value.resize(__size);
	for(int i=0; i < __size;i++)
		src >> value[i];
	return src;
}

//list
template<typename T>
inline ByteArray &operator<<(ByteArray& dst,const std::list<T>& value)
{
	uint16_t _size = value.size();
	dst<<_size;
	for (typename std::list<T>::const_iterator iter = value.begin();iter!=value.end();++iter)
	{
		dst<< *iter;
	}
	return dst;
}

template<typename T>
inline ByteArray &operator>>(ByteArray& src,std::list<T>& value)
{
	uint16_t __size;
	src>>__size;
	value.clear();
	T a;
	while(__size>0)
	{			
		src>>a;
		value.push_back(a);
		__size--;
	}
	return src;
}

//set
template<typename T>
inline ByteArray &operator<<(ByteArray& dst,const std::set<T>& value)
{
	uint16_t _size = value.size();
	dst<<_size;
	for (typename std::set<T>::const_iterator iter = value.begin();iter!=value.end();++iter)
	{
		dst<< *iter;
	}
	return dst;
}

template<typename T>
inline ByteArray &operator>>(ByteArray& src,std::set<T>& value)
{
	uint16_t __size;
	src>>__size;
	value.clear();
	T a;
	while(__size>0)
	{			
		src>>a;
		value.insert(a);
		__size--;
	}
	return src;
}

//deque
template<typename T>
inline ByteArray &operator<<(ByteArray& dst,const std::deque<T>& value)
{
	uint16_t _size = value.size();
	dst<<_size;
	for (typename std::deque<T>::const_iterator iter = value.begin();iter!=value.end();++iter)
	{
		dst<< *iter;
	}
	return dst;
}

template<typename T>
inline ByteArray &operator>>(ByteArray& src,std::deque<T>& value)
{
	uint16_t __size;
	src>>__size;
	value.clear();
	T a;
	while(__size>0)
	{			
		src>>a;
		value.push_back(a);
		__size--;
	}
	return src;
}


//map
template<typename KEY_TYPE,typename VALUE_TYPE>
inline ByteArray &operator<<(ByteArray& dst, const std::map<KEY_TYPE,VALUE_TYPE>& value)
{
	uint16_t _size = value.size();
	dst<<_size;
	for (typename std::map<KEY_TYPE,VALUE_TYPE>::const_iterator iter = value.begin();iter!=value.end();++iter)
	{
		dst << iter->first;
		dst << iter->second;
	}
	return dst;
}

template<typename KEY_TYPE,typename VALUE_TYPE>
inline ByteArray &operator>>(ByteArray& src,std::map<KEY_TYPE,VALUE_TYPE>& value)
{
	uint16_t __size;
	src>>__size;
	value.clear();
	KEY_TYPE i;
	while(__size>0)
	{			
		src>>i;
		//VALUE_TYPE a;
		src >> value[i] ;
		//= a;
		__size--;
	}
	return src;
}


} /* namespace core_obj */
#endif /* BYTEARRAY_H_ */

module;
#include <algorithm>
#include <array>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <iterator>
#include <limits>
#include <print>
#include <fstream>
#include <span>
#include <stdexcept>
#include <optional>
#include <vector>
#include <ranges>
export module rostam;


export class rostam{
    
    struct TSHeader {
        bool syncByte = true;
        bool TEI = false; // Transport Error Indicator
        int PID = 0;
        int TSC = 0;
        int AFC = 0;
        int  CC = 0;
        bool hasPayload = 0;
        int payloadOffset = 0;
        int payloadLength = 0;
        std::span<const int> payload;
    };
    struct EQHeader{
        int version=0;
        int flags=0;
        std::size_t filename_length=0;
        std::size_t file_size = 0;
    };
    
    public:
    rostam(const std::function<void (int)> progress_callback = nullptr):
    m_state(STATE::STATE_SEARCHING_FOR_HEADER),
    currentEQHeaderBytesRead(0),
    previousPacketMagicBytePatternIndex(0),
    m_debug(true),
    bufferLength(0),
    m_file_data_read(0),
    m_progress_callback(progress_callback)
    {
        
    }
    
    void extract (const std::filesystem::path& input, const std::filesystem::path& output)
    {
        constexpr static auto ts_packet_size = 188uz;
        m_output_path = output;
        const auto input_ts_size =  std::filesystem::file_size(input);
        std::ifstream input_ts_file (input,std::ios::binary);
        std::array <int,ts_packet_size*20> input_chunk; //lets get 20 packets at once

        for(auto i = 0uz ; i < input_ts_size / (ts_packet_size*20) ; i++)
        {
            std::ranges::generate (input_chunk,[&input_ts_file]{return input_ts_file.get();});
            std::ranges::for_each(input_chunk|std::views::chunk(ts_packet_size),std::bind_front(
                &rostam::parse_ts_packets,this));
            if(i%20 == 0)
            {
                // get percent value and force it to be 99 after the extraction we call the callback with 100.
                if(m_progress_callback)m_progress_callback(std::min<int>(i*100/static_cast<float>(input_ts_size / (ts_packet_size*20)),99));
            }
        }
        if(m_progress_callback)m_progress_callback(100);
    }

    private:

    auto reset_state(const bool no_log) -> void
    {
        std::println("Reset Called");
        if (!no_log) 
        {
           std::println("Scanning for files to extract...");
        }
        m_state = rostam::STATE::STATE_SEARCHING_FOR_HEADER;
        this->eQHeader = {0,0,0,0};
        m_buffer.erase(m_buffer.begin(),m_buffer.end());
        this->bufferLength = 0; // How much has been read into the buffer
        this->filename.erase(0); // Filename of current file being extracted (if any)
        // this.fileData = []; // Data that has been read for current file. Array of Uint8Arrays (I think its never used)
        this->m_file_data_read = 0uz; // How much file data has been read so far
        // this.cancelFlag = false;
            std::filesystem::path m_output_path;
        
        currentEQHeader.erase(currentEQHeader.begin(),currentEQHeader.end());
        currentEQHeaderBytesRead = 0;
        previousPacketMagicBytePatternIndex = 0;
    }


    auto parseEQHeader(const std::span<const int> eq_header) const -> EQHeader //OK, Works properly
    {
        // this was from the js file this.currentEQHeader = Buffer.alloc(EQSAT_HEADER_SIZE_WITHOUT_MAGIC_BYTES);
        // which then will be passed to this function. buffer is part of the Buffer from nodejs.
        // std::println("parseEQheader called");
        const auto bytes_to_uint64 = [](const std::span<const int> byte_span, const std::size_t offset){
            const static auto shift_left_full64 = 56;
            auto value = 0ul;
            for (int i = 0; i < 8; ++i) {
                value = (value >> 8) | (static_cast<unsigned long>(byte_span[i+offset]) << shift_left_full64);
                // std::print("{:X} - ",buffer[i + offset]);
            }
            // std::println("\n-> so the value is: 0x{0:X} ({0})",value);
            return value;
        };
        
        return {
            eq_header[0], // The first 12 bytes are the magic bytes
            eq_header[1], // flags
            bytes_to_uint64(eq_header,2),
            bytes_to_uint64(eq_header,10) 
        };
    }

    auto getPESAndAC3HeaderSize(const std::span<const int> packet, const TSHeader& header) const -> std::size_t
    {
        const auto payloadSize = header.payloadLength;
        
        // TODO
        // Note that we are cheating here.
        // PES headers don't have to be aligned with the start of the MPEG-TS packet payloads
        // but in the streams we generate with ffmpeg they are always aligned
        if(payloadSize < 6) return 0;
    
        const auto offset = header.payloadOffset;
        
        // Check for PES start code prefix
        if(packet[offset] == 0x00 and packet[offset+1] == 0x00 and packet[offset+2] == 0x01) 
        {
            // This is the PES Stream ID used for DVB type AC-3 streams
            if(const auto streamID = packet[offset + 3];
            streamID != 0xbd) 
            {
                return 0;
            }
    
            // Audio streams have at least 9 bytes of header
            if(payloadSize < 9) 
            {
                return 0;
            }
    
            // How many more bytes of optional PES header are still left
            const auto pesHeaderLeft = packet[offset + 8];
            const auto pesHeaderSize = 9 + pesHeaderLeft;
    
            if(pesHeaderSize > payloadSize) 
            {
                return 0;
            } 
            else 
            {
                if(packet[offset + pesHeaderSize] == 0x0B and packet[offset + pesHeaderSize + 1] == 0x77) 
                {
                    if(pesHeaderSize + 7 > payloadSize) 
                        return 0;
                    return pesHeaderSize + 7; // AC-3 header is always 7 bytes
                }
            return 0;
            }
        }
        return 0;
    } // getPESAndAC3HeaderSize(packet, header)


    auto parseTSHeader(const std::span<const int> packet) const -> std::optional<TSHeader> 
    {
        constexpr auto static this_pid = 6530;
        TSHeader header;

        if(packet[0] != 0x47) {
            header.syncByte = false;
            return header;
        }

        // Packet corrupted (FEC unable to correct)
	    // packet[1].bit[15] == 1 means error (but packet[1] can only contain 8 bits. What is this?)
        if(packet[1] & (1 << 15)) //this looks weired I predict that it would never run. I'll keep it there for now.
        {
            header.TEI = true;
            return header;
        }

        header.PID = ((0b00011111 & packet[1]) << 8) | packet[2];
        // std::println("byte[1] is: 0x{0:x} (0b{0:B}) and byte[2] is: 0x{1:x} (0b{1:B}) so header.PID = {2} ({2:B})",packet[1],packet[2],header.PID);

        if(header.PID != this_pid) return std::nullopt; //PID didn't match so nothing will be parsed

        // Transport Scrambling Control (non-zero means scrambled)
        header.TSC = (packet[3] & 0b11000000) >> 6;

        // Adaptation field control
        header.AFC = (packet[3] & 0b00110000) >> 4;

        // Continuity counter
        header.CC = packet[3] & 0b00001111;

        if(header.AFC == 1 and packet.size() > 4) 
        {
            header.hasPayload = true;
            header.payloadOffset = 4;
        } 
        else if(header.AFC == 3 && packet.size() > 5) 
        {
            header.hasPayload = true;
            header.payloadOffset = 4 + 1 + packet[4];
        } 
        else 
        {
            header.hasPayload = false;
        }
        if(header.hasPayload) 
        {
            header.payloadLength = 188 - header.payloadOffset;

            const auto pesAndAC3HeaderSize = getPESAndAC3HeaderSize(packet, header);
            // TODO not the prettiest to pretend the PES and AC-3 headers
            // are part of the MPEG-TS header but it prevents us modifying a bunch of code
            header.payloadOffset += pesAndAC3HeaderSize;
            header.payloadLength -= pesAndAC3HeaderSize;

            // Creates a view on the packet buffer 
            if(static_cast<std::size_t>(header.payloadOffset) > packet.size())throw std::logic_error("bad subspan");
            header.payload = packet.subspan(header.payloadOffset, header.payloadLength);
        }
        // if(m_debug)std::println("header TSC : {}\nheader AFC: {}\nhreader CC: {}\nheader:payload:\n{}",header.TSC,header.AFC,header.CC,header.payload|std::views::transform([](const auto v){return std::format("{:0>2x}",v);}));
        return header;
    } // parseTSHeader(packet) 


    auto findMagicBytes(const std::span<const int> payload) -> long long 
    {
        if(payload.size() >= 12) 
        {
            const auto magic_bytes_search = std::ranges::search(payload,EQSAT_MAGIC_BYTES);
            const auto magicBytesOffset = std::distance(payload.begin(),magic_bytes_search.begin());
            
            if(not magic_bytes_search.empty())
            {
                std::println("found magic bytes in offset: {}", magicBytesOffset);
                // printHex("Payload: ", payload.subarray(magicBytesOffset, magicBytesOffset+16), 16);
                previousPacketMagicBytePatternIndex = 0;
                return magicBytesOffset + EQSAT_MAGIC_BYTES.size();
            }
        }

        auto patternIndex = previousPacketMagicBytePatternIndex;// || 0;
        previousPacketMagicBytePatternIndex = 0;
        
        for(auto searchIndex=0uz; searchIndex < payload.size(); searchIndex++) 
        {
            // if(m_debug)std::println("looking for byte {} at {} which is {}", patternIndex, searchIndex, payload[searchIndex]);
            if(payload[searchIndex] == EQSAT_MAGIC_BYTES[patternIndex]) 
            {
                patternIndex++;
                if(searchIndex == 0 && patternIndex == 0) std::println("-- found {} bytes", patternIndex);
                if(patternIndex >= EQSAT_MAGIC_BYTES.size())
                {
                    // Return the offset of the first byte after the pattern
                    return searchIndex + 1;
                }
            } 
            else if(payload[searchIndex] == EQSAT_MAGIC_BYTES[0]) 
            {
                patternIndex = 1;
                // std::println("-- found 1 byte");
            } 
            else 
            {
                patternIndex = 0;
            }
        }
        previousPacketMagicBytePatternIndex = patternIndex;

        return -1;
    }

    //changes currentEQHeader
    auto checkForEQHeader(const TSHeader& tsHeader) -> long long 
    {
        constexpr auto EQSAT_HEADER_SIZE = 30; // eQsat v2 header is 30 bytes long
        constexpr auto EQSAT_HEADER_SIZE_WITHOUT_MAGIC_BYTES = EQSAT_HEADER_SIZE - EQSAT_MAGIC_BYTES.size();
        if(!tsHeader.hasPayload) return -1;
        const auto payload = tsHeader.payload;

        // If we already read some of the header bytes from the previous packet
        // then try to read the rest, or at least some more header bytes from this packet
        if(not this->currentEQHeader.empty()) 
        {
            const auto bytesRemaining = EQSAT_HEADER_SIZE_WITHOUT_MAGIC_BYTES - currentEQHeaderBytesRead;
            const auto to_copy = std::min(payload.size(), bytesRemaining);
            std::println("trying to copy from payload to currentEQHeader in checkForEQHeader() in the first if-statement to_copy is: {} and payload.size() is: {}",to_copy,payload.size());
            if(to_copy > payload.size())throw std::logic_error("to_copy > payload.size()");
            std::ranges::copy_n(payload.begin(), to_copy, std::back_inserter(currentEQHeader));
            // buffer.copy (target              , targetStart                  , sourceStart, sourceEnd);
            // payload.copy(this.currentEQHeader, this.currentEQHeaderBytesRead, 0, to_copy);
            if(to_copy < bytesRemaining) return -1;
            return to_copy;
        }
        
        const auto header_offset = findMagicBytes(payload);
        if(header_offset < 0) return -1;

        this->currentEQHeader = std::vector<int>();

        // If the beginning of the header (after the magic bytes) was found
        // but the header is bigger than the remaining payload of the packet
        // meaning that the header spans into the next packet
        if(header_offset + EQSAT_HEADER_SIZE_WITHOUT_MAGIC_BYTES + 1 > payload.size()) {
            const auto toRead = payload.size() - header_offset;
            // Copy the bytes beginning at the header offset in the payload to this.currentEQHeader
            std::println("trying to copy data in checkForEQHeader() second if-statement");
            std::ranges::copy(payload.cbegin()+header_offset,payload.cend(),std::back_inserter(currentEQHeader));
            // buffer.copy (target,         targetStart , sourceStart);
            // payload.copy(this.currentEQHeader, 0     , headerOffset);
            currentEQHeaderBytesRead = toRead;
            return -1;
        }
        std::println("trying to copy data in checkForEQHeader() no cond");
        std::ranges::copy_n(payload.begin()+header_offset,EQSAT_HEADER_SIZE_WITHOUT_MAGIC_BYTES,std::back_inserter(currentEQHeader));
        std::println("Done current eQHeader is: {}",currentEQHeader);
        //buffer. copy( target,        targetStart, sourceStart, sourceEnd )
        //payload.copy(this.currentEQHeader, 0, headerOffset, headerOffset + EQSAT_HEADER_SIZE_WITHOUT_MAGIC_BYTES);

        return header_offset + EQSAT_HEADER_SIZE_WITHOUT_MAGIC_BYTES;
    } 


    auto parse_ts_packets(const std::span<const int> packet) -> void
    {
        // constexpr auto packet_size = 188uz;
        // if(packet.size() < 4) return ;
        if(packet.at(0) != 0x47)throw std::logic_error("Out of sync detected");
        // If the packet is smaller than the MPEG-TS packet header, skip
        if(packet.size() < 4) return;

        const auto ts_header = this->parseTSHeader(packet); // parse MPEG-TS header
        //if(tsHeader.has_value())std::println("{}",tsHeader->payload);
        // If parseTSHeader() returns false then the PID didn't match
        // or the header failed to parse so skip the packet
        if(!ts_header) return;

        // This is incremented every time some of the current packet is consumed
        // e.g. by reading the header or filename
        auto curPayloadOffset = 0ll;

        if(m_state == rostam::STATE::STATE_SEARCHING_FOR_HEADER) 
        {
            curPayloadOffset = this->checkForEQHeader(*ts_header);
            if(curPayloadOffset >= 0) 
            {
                std::println("Found beginning of new file in offset {}",curPayloadOffset);
                this->eQHeader = parseEQHeader(this->currentEQHeader);
                this->currentEQHeader = {}; //set to undefined {} is the closeest
                
                if(m_debug) {
                    std::println("Header file size: {}", this->eQHeader.file_size);
                }
                if(eQHeader.filename_length >= std::numeric_limits<uint8_t>::max()) throw std::logic_error("too many allocations in SEARCHING_FOR_HEADERS cond");
                // std::println("parse_ts_packates() allocating filename_length: {}",eQHeader.filename_length);
                this->m_buffer = std::vector<unsigned char>(eQHeader.filename_length); //ME: throws -> FIXED
                bufferLength = 0;
                
                //this->bufferLength = 0;
                // tsHeader.payloadOffset += EQSAT_HEADER_SIZE;
                // tsHeader.payloadLength -= EQSAT_HEADER_SIZE;
                std::println("Changed the state-machine to STATE_READING_FILENAME");
                m_state = rostam::STATE::STATE_READING_FILENAME;
            }
        }
        if(m_state == rostam::STATE::STATE_READING_FILENAME) 
        {   
            //std::println("parse_ts_packets() -> state machine changed to: STATE_READING_FILENAME");
            if(!ts_header->hasPayload) return;
            //std::println("we have payload!");
            const auto toCopyMax = this->m_buffer.size() - this->bufferLength;
            const auto toCopy = std::min(ts_header->payload.size() - static_cast<std::size_t>(curPayloadOffset), toCopyMax);
            std::ranges::copy_n(ts_header->payload.begin()+curPayloadOffset,toCopy,m_buffer.begin());

            //tsHeader->payload.copy(this.buffer, this.bufferLength, curPayloadOffset, curPayloadOffset + toCopy);
            this->bufferLength += toCopy;
            curPayloadOffset += toCopy;

            if(this->bufferLength >= this->m_buffer.size())
            {
                m_state = rostam::STATE:: STATE_READING_FILE;

                std::ranges::copy(m_buffer,std::back_inserter(filename));
                std::println("Extracting file:  {}" , filename);

                // TODO is Number big enough?
                m_buffer = std::vector<unsigned char>(this->eQHeader.file_size);
                bufferLength = 0;

                // If an output dir is specified and this is running in node.js
                // then write files to the dir as they are extracted
                // MY TODO: HARDCODED ADDRESS YOU NEED TO ADD IT TO CONSTRUCTOR PARAMS
                const auto output_file_path = m_output_path/(filename + ".part");
                
                //try {
                // Open file for writing
                // TODO change to async open call
                m_current_output_file.open(output_file_path,std::ios::binary);
                if(!m_current_output_file)std::runtime_error("Could not open the output file.");
                    
                // this.curOutFile = fs.openSync(filePath, 'w', 0o640);
                // console.log("Opened file for writing!");
                //} 
                // catch(...) {std::println("error");}
            }        
        } 
        if(m_state == rostam::STATE::STATE_READING_FILE) 
        {
            if(!ts_header->hasPayload) return;
            
            const auto to_read = std::min(ts_header->payload.size() - static_cast<std::size_t>(curPayloadOffset), this->eQHeader.file_size - this->m_file_data_read);

            // subspan makes a span, not a copy
            // but since that's just the payload + 4 bytes 
            // it seems like a good trade-off to avoid an extra copy operation
            
            // MY TODO : FIGURE THIS OUT
            const auto chunk = ts_header->payload.subspan(curPayloadOffset, to_read);
            
            if(this->m_current_output_file) 
            {
                // MY TODO: this is really inefficent I need to rip a lot of things to convert the payload to unsigned char
                //it's safe at least
                for(const auto s : chunk)m_current_output_file.put(s);
                // fs.writeSync(this->m_current_output_file, chunk);
                
                //}catch(...) {
                
                //return cb(err); 
                //}
            }
            /*else 
            {
                //MY TODO: MAKE IT WORK AND WHY DO WE NEED THIS
                // this.buffer.set(this.fileDataRead, chunk);
            }*/
            
            m_file_data_read += to_read;
            
            if(m_file_data_read >= this->eQHeader.file_size) 
            {
                // this.curOutFile = null; // WTF is this
                m_current_output_file.close();
                std::filesystem::rename(m_output_path/(filename+".part"),m_output_path/filename);
                std::println("Completed extraction of file:\n  {}", this->filename);
                reset_state(false);    
            }
        }
    }
    

    

    private:
    enum class STATE {
        STATE_SEARCHING_FOR_HEADER = 0, // Searching for eQsat header
        STATE_READING_HEADER = 1, // Found the header, now reading it
        STATE_READING_FILENAME = 2, // Done reading header, now reading filename
        STATE_READING_FILE = 3 // Done reading filename, now reading file
    };
    std::filesystem::path m_output_path;
    STATE m_state;
    EQHeader eQHeader;
    std::vector<unsigned char>m_buffer; // This is the final container for the output file. 
    std::vector<int> currentEQHeader;
    std::size_t currentEQHeaderBytesRead;
    std::size_t previousPacketMagicBytePatternIndex;
    std::ofstream m_current_output_file;
    std::string filename;
    const bool m_debug;
    std::size_t bufferLength;
    std::size_t m_file_data_read;
    static constexpr auto EQSAT_MAGIC_BYTES = std::to_array({0xCA, 0xFE, 0xC0, 0xDE, 0xF0, 0x0D, 0xCA, 0xFE, 0xC0, 0xDE, 0xF0, 0x0D});
    static constexpr auto EQSAT_HEADER_SIZE = 30uz; // eQsat v2 header is 30 bytes long ; 
    const std::function<void(int)> m_progress_callback;
};






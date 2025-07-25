#pragma once

#include "claude_draw/serialization/binary_format.h"
#include "claude_draw/serialization/binary_deserializer.h"
#include "claude_draw/serialization/binary_serializer.h"
#include <functional>
#include <memory>
#include <unordered_map>
#include <map>
#include <vector>

namespace claude_draw {
namespace serialization {

/**
 * @brief Version information for migration
 */
struct Version {
    uint16_t major;
    uint16_t minor;
    
    Version(uint16_t maj = 0, uint16_t min = 0) : major(maj), minor(min) {}
    
    bool operator<(const Version& other) const {
        return (major < other.major) || (major == other.major && minor < other.minor);
    }
    
    bool operator==(const Version& other) const {
        return major == other.major && minor == other.minor;
    }
    
    bool operator<=(const Version& other) const {
        return *this < other || *this == other;
    }
    
    std::string to_string() const {
        return std::to_string(major) + "." + std::to_string(minor);
    }
};

/**
 * @brief Migration result
 */
struct MigrationResult {
    bool success = true;
    Version from_version;
    Version to_version;
    std::vector<std::string> messages;
    std::vector<std::string> warnings;
    
    void add_message(const std::string& msg) {
        messages.push_back(msg);
    }
    
    void add_warning(const std::string& warning) {
        warnings.push_back(warning);
    }
    
    void add_error(const std::string& error) {
        success = false;
        messages.push_back("ERROR: " + error);
    }
};

/**
 * @brief Interface for version-specific format handlers
 */
class IFormatHandler {
public:
    virtual ~IFormatHandler() = default;
    
    /**
     * @brief Get the version this handler supports
     */
    virtual Version get_version() const = 0;
    
    /**
     * @brief Read an object of the specified type
     */
    virtual bool read_object(BinaryReader& reader, TypeId type, void* output) = 0;
    
    /**
     * @brief Write an object of the specified type
     */
    virtual bool write_object(BinaryWriter& writer, TypeId type, const void* data) = 0;
    
    /**
     * @brief Check if this handler can read files from the specified version
     */
    virtual bool can_read_from(const Version& version) const = 0;
};

/**
 * @brief Migration function type
 */
using MigrationFunction = std::function<MigrationResult(
    const std::vector<uint8_t>& input_data,
    std::vector<uint8_t>& output_data)>;

/**
 * @brief Version migration manager
 */
class VersionMigrationManager {
public:
    static VersionMigrationManager& instance() {
        static VersionMigrationManager instance;
        return instance;
    }
    
    /**
     * @brief Register a format handler for a specific version
     */
    void register_handler(const Version& version, 
                         std::unique_ptr<IFormatHandler> handler) {
        handlers_[version] = std::move(handler);
    }
    
    /**
     * @brief Register a migration function between two versions
     */
    void register_migration(const Version& from, const Version& to,
                          MigrationFunction migration) {
        migrations_[{from, to}] = migration;
    }
    
    /**
     * @brief Get handler for a specific version
     */
    IFormatHandler* get_handler(const Version& version) {
        auto it = handlers_.find(version);
        return (it != handlers_.end()) ? it->second.get() : nullptr;
    }
    
    /**
     * @brief Find migration path between versions
     */
    std::vector<Version> find_migration_path(const Version& from, const Version& to) {
        if (from == to) return {from};
        
        // Simple BFS to find shortest migration path
        std::vector<Version> path;
        std::map<std::pair<uint16_t, uint16_t>, Version> parent;
        std::vector<Version> queue = {from};
        parent[{from.major, from.minor}] = from;
        
        while (!queue.empty()) {
            Version current = queue.front();
            queue.erase(queue.begin());
            
            if (current == to) {
                // Reconstruct path
                Version v = to;
                while (!(v == from)) {
                    path.insert(path.begin(), v);
                    v = parent[{v.major, v.minor}];
                }
                path.insert(path.begin(), from);
                return path;
            }
            
            // Check all possible migrations from current
            for (const auto& [key, func] : migrations_) {
                if (key.first == current && 
                    parent.find({key.second.major, key.second.minor}) == parent.end()) {
                    parent[{key.second.major, key.second.minor}] = current;
                    queue.push_back(key.second);
                }
            }
        }
        
        return {};  // No path found
    }
    
    /**
     * @brief Migrate data between versions
     */
    MigrationResult migrate(const Version& from, const Version& to,
                          const std::vector<uint8_t>& input_data,
                          std::vector<uint8_t>& output_data) {
        MigrationResult result;
        result.from_version = from;
        result.to_version = to;
        
        if (from == to) {
            output_data = input_data;
            result.add_message("No migration needed - same version");
            return result;
        }
        
        auto path = find_migration_path(from, to);
        if (path.empty()) {
            result.add_error("No migration path found from " + from.to_string() + 
                           " to " + to.to_string());
            return result;
        }
        
        // Apply migrations along the path
        std::vector<uint8_t> current_data = input_data;
        
        for (size_t i = 0; i < path.size() - 1; ++i) {
            Version step_from = path[i];
            Version step_to = path[i + 1];
            
            auto it = migrations_.find({step_from, step_to});
            if (it == migrations_.end()) {
                result.add_error("Missing migration from " + step_from.to_string() + 
                               " to " + step_to.to_string());
                return result;
            }
            
            std::vector<uint8_t> migrated_data;
            auto step_result = it->second(current_data, migrated_data);
            
            if (!step_result.success) {
                result.add_error("Migration failed at step " + step_from.to_string() + 
                               " to " + step_to.to_string());
                for (const auto& msg : step_result.messages) {
                    result.add_message("  " + msg);
                }
                return result;
            }
            
            result.add_message("Migrated from " + step_from.to_string() + 
                             " to " + step_to.to_string());
            current_data = std::move(migrated_data);
        }
        
        output_data = std::move(current_data);
        result.add_message("Migration complete");
        return result;
    }
    
private:
    VersionMigrationManager() = default;
    
    // Version hash function
    struct VersionHash {
        size_t operator()(const Version& v) const {
            return std::hash<uint32_t>()(
                (static_cast<uint32_t>(v.major) << 16) | v.minor);
        }
    };
    
    // Version pair hash function
    struct VersionPairHash {
        size_t operator()(const std::pair<Version, Version>& p) const {
            auto h1 = VersionHash()(p.first);
            auto h2 = VersionHash()(p.second);
            return h1 ^ (h2 << 1);
        }
    };
    
    std::unordered_map<Version, std::unique_ptr<IFormatHandler>, VersionHash> handlers_;
    std::unordered_map<std::pair<Version, Version>, MigrationFunction, VersionPairHash> migrations_;
};

/**
 * @brief Base implementation of format handler
 */
class FormatHandlerBase : public IFormatHandler {
public:
    FormatHandlerBase(uint16_t major, uint16_t minor) 
        : version_(major, minor) {}
    
    Version get_version() const override {
        return version_;
    }
    
    bool can_read_from(const Version& version) const override {
        // By default, can read from same major version, any minor
        return version.major == version_.major && version.minor <= version_.minor;
    }
    
protected:
    Version version_;
};

/**
 * @brief Current version format handler
 */
class CurrentFormatHandler : public FormatHandlerBase {
public:
    CurrentFormatHandler() 
        : FormatHandlerBase(CURRENT_VERSION_MAJOR, CURRENT_VERSION_MINOR) {}
    
    bool read_object(BinaryReader& reader, TypeId type, void* output) override {
        // Implement reading for current format version
        // This would use the standard deserialization functions
        return true;
    }
    
    bool write_object(BinaryWriter& writer, TypeId type, const void* data) override {
        // Implement writing for current format version
        // This would use the standard serialization functions
        return true;
    }
};

/**
 * @brief Backward compatible reader
 */
class BackwardCompatibleReader {
public:
    /**
     * @brief Read file with automatic version migration
     */
    static std::vector<uint8_t> read_file(const std::string& filename,
                                        MigrationResult* migration_result = nullptr) {
        // Read file header first
        std::ifstream file(filename, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + filename);
        }
        
        FileHeader header;
        file.read(reinterpret_cast<char*>(&header), sizeof(FileHeader));
        
        // Convert from file endianness
        header.magic = from_file_endian(header.magic);
        header.version_major = from_file_endian(header.version_major);
        header.version_minor = from_file_endian(header.version_minor);
        header.total_size = from_file_endian(header.total_size);
        
        if (!header.validate()) {
            throw std::runtime_error("Invalid file header");
        }
        
        Version file_version(header.version_major, header.version_minor);
        Version current_version(CURRENT_VERSION_MAJOR, CURRENT_VERSION_MINOR);
        
        // Read entire file
        file.seekg(0);
        std::vector<uint8_t> file_data(header.total_size);
        file.read(reinterpret_cast<char*>(file_data.data()), header.total_size);
        
        // Check if migration is needed
        if (file_version == current_version) {
            if (migration_result) {
                migration_result->success = true;
                migration_result->from_version = file_version;
                migration_result->to_version = current_version;
                migration_result->add_message("No migration needed");
            }
            return file_data;
        }
        
        // Migrate to current version
        std::vector<uint8_t> migrated_data;
        auto result = VersionMigrationManager::instance().migrate(
            file_version, current_version, file_data, migrated_data);
        
        if (migration_result) {
            *migration_result = result;
        }
        
        if (!result.success) {
            throw std::runtime_error("Migration failed: " + 
                                   (result.messages.empty() ? "Unknown error" : result.messages.back()));
        }
        
        return migrated_data;
    }
    
    /**
     * @brief Create reader from file with automatic migration
     */
    static std::unique_ptr<BinaryReader> from_file(const std::string& filename,
                                                  MigrationResult* migration_result = nullptr) {
        auto data = read_file(filename, migration_result);
        return std::make_unique<BinaryReader>(std::move(data));
    }
};

/**
 * @brief Helper to register standard migrations
 */
class StandardMigrations {
public:
    static void register_all() {
        auto& manager = VersionMigrationManager::instance();
        
        // Register current version handler
        manager.register_handler(
            Version(CURRENT_VERSION_MAJOR, CURRENT_VERSION_MINOR),
            std::make_unique<CurrentFormatHandler>());
        
        // Example: Migration from 1.0 to 1.1
        // This would handle any format changes between minor versions
        manager.register_migration(
            Version(1, 0), Version(1, 1),
            [](const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
                MigrationResult result;
                result.from_version = Version(1, 0);
                result.to_version = Version(1, 1);
                
                // For now, just copy data (no format changes yet)
                output = input;
                result.add_message("Migrated from 1.0 to 1.1 (no changes)");
                
                return result;
            });
    }
};

} // namespace serialization
} // namespace claude_draw
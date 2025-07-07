/*
 * Flox Engine
 * Developed by FLOX Foundation (https://github.com/FLOX-Foundation)
 *
 * Copyright (c) 2025 FLOX Foundation
 * Licensed under the MIT License. See LICENSE file in the project root for full
 * license information.
 */

#pragma once

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

// NUMA support detection - use CMake-defined flag
#if defined(FLOX_NUMA_LIBRARY_LINKED) && FLOX_NUMA_LIBRARY_LINKED && __has_include(<numaif.h>)
#include <numa.h>
#include <numaif.h>
#define FLOX_NUMA_AVAILABLE 1
#else
#define FLOX_NUMA_AVAILABLE 0
// Define NUMA constants for fallback
#ifndef MPOL_DEFAULT
#define MPOL_DEFAULT 0
#define MPOL_PREFERRED 1
#endif
// Provide fallback implementations
static inline int set_mempolicy(int mode, const unsigned long* nodemask, unsigned long maxnode)
{
  (void)mode;
  (void)nodemask;
  (void)maxnode;
  return -1;
}
#endif
#else
#define FLOX_NUMA_AVAILABLE 0
// Define fallback for non-Linux platforms
#ifndef MPOL_DEFAULT
#define MPOL_DEFAULT 0
#define MPOL_PREFERRED 1
#endif
static inline int set_mempolicy(int mode, const unsigned long* nodemask, unsigned long maxnode)
{
  (void)mode;
  (void)nodemask;
  (void)maxnode;
  return -1;
}
#endif

namespace flox::performance
{

/**
 * @brief CPU affinity and thread pinning utilities for low-latency performance optimization
 * 
 * This class provides functionality to:
 * - Pin threads to specific CPU cores
 * - Set thread priorities
 * - Isolate critical threads from OS interrupts
 * - Get CPU topology information
 * - NUMA-aware thread and memory placement
 */
class CpuAffinity
{
 public:
  /**
   * @brief NUMA node information
   */
  struct NumaNode
  {
    int nodeId;
    std::vector<int> cpuCores;
    size_t totalMemoryMB;
    size_t freeMemoryMB;
  };

  /**
   * @brief NUMA topology information
   */
  struct NumaTopology
  {
    std::vector<NumaNode> nodes;
    int numNodes;
    bool numaAvailable;
  };

  /**
     * @brief Pin current thread to specific CPU core
     * @param coreId CPU core ID (0-based)
     * @return true if successful, false otherwise
     */
  static bool pinToCore(int coreId)
  {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0)
    {
      return true;
    }
    else
    {
      return false;
    }
#else
    (void)coreId;
    return false;
#endif
  }

  /**
     * @brief Pin a thread to specific CPU core
     * @param thread Thread to pin
     * @param coreId CPU core ID (0-based)
     * @return true if successful, false otherwise
     */
  static bool pinToCore(std::thread& thread, int coreId)
  {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(coreId, &cpuset);

    if (pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset) == 0)
    {
      return true;
    }
    else
    {
      return false;
    }
#else
    (void)thread;
    (void)coreId;
    return false;
#endif
  }

  /**
     * @brief Set thread priority for real-time performance
     * @param priority Priority level (1-99, higher = more priority)
     * @return true if successful, false otherwise
     */
  static bool setRealTimePriority(int priority = 80)
  {
#ifdef __linux__
    struct sched_param param;
    param.sched_priority = priority;

    if (sched_setscheduler(0, SCHED_FIFO, &param) == 0)
    {
      return true;
    }
    else
    {
      return false;
    }
#else
    (void)priority;
    return false;
#endif
  }

  /**
     * @brief Set thread priority for a specific thread
     * @param thread Thread to set priority for
     * @param priority Priority level (1-99, higher = more priority)
     * @return true if successful, false otherwise
     */
  static bool setRealTimePriority(std::thread& thread, int priority = 80)
  {
#ifdef __linux__
    struct sched_param param;
    param.sched_priority = priority;

    if (pthread_setschedparam(thread.native_handle(), SCHED_FIFO, &param) == 0)
    {
      return true;
    }
    else
    {
      return false;
    }
#else
    (void)thread;
    (void)priority;
    return false;
#endif
  }

  /**
     * @brief Get number of available CPU cores
     * @return Number of CPU cores
     */
  static int getNumCores()
  {
    return std::thread::hardware_concurrency();
  }

  /**
     * @brief Get list of isolated CPU cores (not used by OS)
     * @return Vector of isolated core IDs
     */
  static std::vector<int> getIsolatedCores()
  {
    std::vector<int> isolatedCores;

#ifdef __linux__
    // Read from /sys/devices/system/cpu/isolated
    auto content = readFile("/sys/devices/system/cpu/isolated");
    if (!content)
    {
      return isolatedCores;
    }

    std::stringstream ss(*content);
    std::string range;

    while (std::getline(ss, range, ','))
    {
      range.erase(range.find_last_not_of(" \n\r\t") + 1);
      range.erase(0, range.find_first_not_of(" \n\r\t"));

      if (range.empty())
        continue;

      size_t dashPos = range.find('-');
      if (dashPos != std::string::npos)
      {
        // Range like "2-5"
        int start = std::stoi(range.substr(0, dashPos));
        int end = std::stoi(range.substr(dashPos + 1));
        for (int i = start; i <= end; ++i)
        {
          isolatedCores.push_back(i);
        }
      }
      else
      {
        // Single core
        isolatedCores.push_back(std::stoi(range));
      }
    }
#endif

    return isolatedCores;
  }

  /**
     * @brief Get current thread's CPU affinity
     * @return Vector of CPU core IDs this thread can run on
     */
  static std::vector<int> getCurrentAffinity()
  {
    std::vector<int> affinity;

#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    if (sched_getaffinity(0, sizeof(cpu_set_t), &cpuset) == 0)
    {
      for (int i = 0; i < CPU_SETSIZE; ++i)
      {
        if (CPU_ISSET(i, &cpuset))
        {
          affinity.push_back(i);
        }
      }
    }
#endif

    return affinity;
  }

  /**
     * @brief Disable CPU frequency scaling for performance cores
     * @return true if successful, false otherwise
     */
  static bool disableCpuFrequencyScaling()
  {
#ifdef __linux__
    // Set CPU governor to performance mode
    int numCores = getNumCores();
    bool success = true;

    for (int i = 0; i < numCores; ++i)
    {
      std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_governor";
      if (!writeFile(path, "performance"))
      {
        success = false;
      }
    }

    return success;
#else
    return false;
#endif
  }

  /**
     * @brief Enable CPU frequency scaling
     * @return true if successful, false otherwise
     */
  static bool enableCpuFrequencyScaling()
  {
#ifdef __linux__
    // Set CPU governor to ondemand mode
    int numCores = getNumCores();
    bool success = true;

    for (int i = 0; i < numCores; ++i)
    {
      std::string path = "/sys/devices/system/cpu/cpu" + std::to_string(i) + "/cpufreq/scaling_governor";
      if (!writeFile(path, "ondemand"))
      {
        success = false;
      }
    }

    return success;
#else
    return false;
#endif
  }

  /**
   * @brief Get recommended core assignment for low-latency workloads
   * @return Struct containing recommended core assignments
   */
  struct CoreAssignment
  {
    std::vector<int> marketDataCores;  // Cores for market data processing
    std::vector<int> strategyCores;    // Cores for strategy execution
    std::vector<int> executionCores;   // Cores for order execution
    std::vector<int> riskCores;        // Cores for risk management
    std::vector<int> generalCores;     // Cores for general tasks

    // Additional information about isolation
    bool hasIsolatedCores = false;
    std::vector<int> allIsolatedCores;
    std::vector<int> criticalCores;  // All cores assigned to critical tasks
  };

  /**
   * @brief Configuration for critical component core assignment
   */
  struct CriticalComponentConfig
  {
    bool preferIsolatedCores;       // Prefer isolated cores for critical tasks
    bool exclusiveIsolatedCores;    // Use isolated cores exclusively for critical tasks
    bool allowSharedCriticalCores;  // Allow multiple critical tasks on same core
    int minIsolatedForCritical;     // Minimum isolated cores needed to use them

    // Priority order for critical component assignment (0 = highest priority)
    std::map<std::string, int> componentPriority;

    // Default constructor with sensible defaults
    CriticalComponentConfig()
        : preferIsolatedCores(true), exclusiveIsolatedCores(true), allowSharedCriticalCores(false), minIsolatedForCritical(1), componentPriority({{"marketData", 0}, {"execution", 1}, {"strategy", 2}, {"risk", 3}})
    {
    }
  };

  /**
   * @brief Get recommended core assignment with enhanced isolated core utilization
   * @param config Configuration for critical component assignment
   * @return CoreAssignment with optimized isolated core usage
   */
  static CoreAssignment getRecommendedCoreAssignment(const CriticalComponentConfig& config = CriticalComponentConfig{})
  {
    CoreAssignment assignment;

    int numCores = getNumCores();
    auto isolatedCores = getIsolatedCores();

    assignment.hasIsolatedCores = !isolatedCores.empty();
    assignment.allIsolatedCores = isolatedCores;

    // Early return if no cores available
    if (numCores == 0)
    {
      return assignment;
    }

    // If insufficient isolated cores or not preferring them, fall back to general assignment
    if (!config.preferIsolatedCores ||
        (int)isolatedCores.size() < config.minIsolatedForCritical)
    {
      return getBasicCoreAssignment(numCores, isolatedCores);
    }

    // Sort components by priority for assignment
    std::vector<std::pair<std::string, int>> sortedComponents;
    for (const auto& [component, priority] : config.componentPriority)
    {
      sortedComponents.emplace_back(component, priority);
    }
    std::sort(sortedComponents.begin(), sortedComponents.end(),
              [](const auto& a, const auto& b)
              { return a.second < b.second; });

    // Assign critical components to isolated cores based on priority
    std::vector<int> availableIsolated = isolatedCores;
    std::vector<int>* targetCores = nullptr;

    for (const auto& [component, priority] : sortedComponents)
    {
      if (availableIsolated.empty() && !config.allowSharedCriticalCores)
      {
        break;
      }

      // Get target vector for this component
      if (component == "marketData")
        targetCores = &assignment.marketDataCores;
      else if (component == "execution")
        targetCores = &assignment.executionCores;
      else if (component == "strategy")
        targetCores = &assignment.strategyCores;
      else if (component == "risk")
        targetCores = &assignment.riskCores;
      else
        continue;

      if (targetCores && !availableIsolated.empty())
      {
        targetCores->push_back(availableIsolated.front());
        assignment.criticalCores.push_back(availableIsolated.front());

        if (!config.allowSharedCriticalCores)
        {
          availableIsolated.erase(availableIsolated.begin());
        }
      }
    }

    // Assign remaining isolated cores to general use if not exclusively for critical tasks
    if (!config.exclusiveIsolatedCores)
    {
      for (int core : availableIsolated)
      {
        assignment.generalCores.push_back(core);
      }
    }

    // Add non-isolated cores to general use
    for (int i = 0; i < numCores; ++i)
    {
      if (std::find(isolatedCores.begin(), isolatedCores.end(), i) == isolatedCores.end())
      {
        assignment.generalCores.push_back(i);
      }
    }

    return assignment;
  }

  /**
   * @brief Get basic core assignment when isolated cores are not used
   * @param numCores Total number of cores
   * @param isolatedCores Vector of isolated cores (for reference)
   * @return CoreAssignment with basic distribution
   */
  static CoreAssignment getBasicCoreAssignment(int numCores, const std::vector<int>& isolatedCores)
  {
    CoreAssignment assignment;
    assignment.hasIsolatedCores = !isolatedCores.empty();
    assignment.allIsolatedCores = isolatedCores;

    if (isolatedCores.empty())
    {
      // No isolated cores, distribute evenly
      for (int i = 0; i < numCores; ++i)
      {
        assignment.generalCores.push_back(i);
      }
    }
    else
    {
      // Use isolated cores for critical tasks when available
      int numIsolated = isolatedCores.size();

      if (numIsolated >= 4)
      {
        // Sufficient isolated cores for specialized assignment
        assignment.marketDataCores.push_back(isolatedCores[0]);
        assignment.strategyCores.push_back(isolatedCores[1]);
        assignment.executionCores.push_back(isolatedCores[2]);
        assignment.riskCores.push_back(isolatedCores[3]);

        assignment.criticalCores = {isolatedCores[0], isolatedCores[1],
                                    isolatedCores[2], isolatedCores[3]};

        // Remaining isolated cores for general use
        for (int i = 4; i < numIsolated; ++i)
        {
          assignment.generalCores.push_back(isolatedCores[i]);
        }
      }
      else
      {
        // Limited isolated cores, prioritize market data and execution
        if (numIsolated >= 1)
        {
          assignment.marketDataCores.push_back(isolatedCores[0]);
          assignment.criticalCores.push_back(isolatedCores[0]);
        }
        if (numIsolated >= 2)
        {
          assignment.executionCores.push_back(isolatedCores[1]);
          assignment.criticalCores.push_back(isolatedCores[1]);
        }
        if (numIsolated >= 3)
        {
          assignment.strategyCores.push_back(isolatedCores[2]);
          assignment.criticalCores.push_back(isolatedCores[2]);
        }
      }

      // Add non-isolated cores for general use
      for (int i = 0; i < numCores; ++i)
      {
        if (std::find(isolatedCores.begin(), isolatedCores.end(), i) == isolatedCores.end())
        {
          assignment.generalCores.push_back(i);
        }
      }
    }

    return assignment;
  }

  /**
   * @brief Pin critical component to its assigned isolated core
   * @param component Component name ("marketData", "execution", "strategy", "risk")
   * @param assignment Core assignment from getRecommendedCoreAssignment
   * @return true if successful, false otherwise
   */
  static bool pinCriticalComponent(const std::string& component, const CoreAssignment& assignment)
  {
    const std::vector<int>* targetCores = nullptr;

    if (component == "marketData")
      targetCores = &assignment.marketDataCores;
    else if (component == "execution")
      targetCores = &assignment.executionCores;
    else if (component == "strategy")
      targetCores = &assignment.strategyCores;
    else if (component == "risk")
      targetCores = &assignment.riskCores;
    else
    {
      return false;
    }

    if (!targetCores || targetCores->empty())
    {
      return false;
    }

    // Pin to the first assigned core for this component
    int coreId = (*targetCores)[0];
    return pinToCore(coreId);
  }

  /**
   * @brief Verify isolation of critical cores
   * @param assignment Core assignment to verify
   * @return true if critical components are properly isolated
   */
  static bool verifyCriticalCoreIsolation(const CoreAssignment& assignment)
  {
    if (!assignment.hasIsolatedCores)
    {
      return false;
    }

    bool allCriticalIsolated = true;

    auto checkCores = [&](const std::vector<int>& cores, const std::string& component)
    {
      for (int coreId : cores)
      {
        bool isIsolated = std::find(assignment.allIsolatedCores.begin(),
                                    assignment.allIsolatedCores.end(), coreId) != assignment.allIsolatedCores.end();

        if (!isIsolated)
        {
          allCriticalIsolated = false;
        }
      }
    };

    checkCores(assignment.marketDataCores, "Market Data");
    checkCores(assignment.executionCores, "Execution");
    checkCores(assignment.strategyCores, "Strategy");
    checkCores(assignment.riskCores, "Risk");

    return allCriticalIsolated;
  }

  /**
   * @brief Get NUMA topology information
   * @return NumaTopology struct containing NUMA node information
   */
  static NumaTopology getNumaTopology()
  {
    NumaTopology topology;
    topology.numaAvailable = false;
    topology.numNodes = 0;

#if defined(__linux__) && FLOX_NUMA_AVAILABLE
    // Check if NUMA is available using proper NUMA API
    if (numa_available() == -1)
    {
      return topology;
    }

    topology.numaAvailable = true;

    // Get maximum NUMA node number
    int maxNode = numa_max_node();
    topology.numNodes = maxNode + 1;

    for (int nodeId = 0; nodeId <= maxNode; ++nodeId)
    {
      NumaNode node;
      node.nodeId = nodeId;

      // Get CPU cores for this node by iterating through all CPUs
      int numCpus = getNumCores();
      for (int cpu = 0; cpu < numCpus; ++cpu)
      {
        if (numa_node_of_cpu(cpu) == nodeId)
        {
          node.cpuCores.push_back(cpu);
        }
      }

      // Get memory information using NUMA API
      long long freeBytes = 0;
      long long totalBytes = numa_node_size64(nodeId, &freeBytes);

      if (totalBytes > 0)
      {
        node.totalMemoryMB = totalBytes / (1024 * 1024);
        node.freeMemoryMB = freeBytes / (1024 * 1024);
      }
      else
      {
        node.totalMemoryMB = 0;
        node.freeMemoryMB = 0;
      }

      topology.nodes.push_back(node);
    }
#elif defined(__linux__)
    // NUMA libraries not available, but this is Linux
    return topology;
#endif

    return topology;
  }

  /**
   * @brief Get NUMA node ID for a specific CPU core
   * @param coreId CPU core ID
   * @return NUMA node ID, -1 if not found or NUMA not available
   */
  static int getNumaNodeForCore(int coreId)
  {
#if defined(__linux__) && FLOX_NUMA_AVAILABLE
    if (numa_available() == -1)
    {
      return -1;
    }
    return numa_node_of_cpu(coreId);
#elif defined(__linux__)
    // Fallback to /sys/ parsing when NUMA library not available
    std::string nodePath = "/sys/devices/system/cpu/cpu" +
                           std::to_string(coreId) + "/topology/physical_package_id";
    auto content = readFile(nodePath);
    if (content)
    {
      try
      {
        return std::stoi(*content);
      }
      catch (...)
      {
        return -1;
      }
    }
#else
    (void)coreId;
#endif
    return -1;
  }

  /**
   * @brief Pin current thread to a NUMA node (any core within the node)
   * @param nodeId NUMA node ID
   * @return true if successful, false otherwise
   */
  static bool pinToNumaNode(int nodeId)
  {
#ifdef __linux__
    auto topology = getNumaTopology();
    if (!topology.numaAvailable)
    {
      return false;
    }

    for (const auto& node : topology.nodes)
    {
      if (node.nodeId == nodeId)
      {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);

        for (int coreId : node.cpuCores)
        {
          CPU_SET(coreId, &cpuset);
        }

        if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == 0)
        {
          return true;
        }
        else
        {
          return false;
        }
      }
    }

    return false;
#else
    (void)nodeId;
    return false;
#endif
  }

  /**
   * @brief Pin thread to a NUMA node
   * @param thread Thread to pin
   * @param nodeId NUMA node ID
   * @return true if successful, false otherwise
   */
  static bool pinToNumaNode(std::thread& thread, int nodeId)
  {
#ifdef __linux__
    auto topology = getNumaTopology();
    if (!topology.numaAvailable)
    {
      return false;
    }

    for (const auto& node : topology.nodes)
    {
      if (node.nodeId == nodeId)
      {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);

        for (int coreId : node.cpuCores)
        {
          CPU_SET(coreId, &cpuset);
        }

        if (pthread_setaffinity_np(thread.native_handle(), sizeof(cpu_set_t), &cpuset) == 0)
        {
          return true;
        }
        else
        {
          return false;
        }
      }
    }

    return false;
#else
    (void)thread;
    (void)nodeId;
    return false;
#endif
  }

  /**
   * @brief Set memory policy to prefer specific NUMA node
   * @param nodeId NUMA node ID to prefer for memory allocation
   * @return true if successful, false otherwise
   */
  static bool setMemoryPolicy(int nodeId)
  {
#if defined(__linux__) && FLOX_NUMA_AVAILABLE
    if (numa_available() == -1)
    {
      return false;
    }

    // Use NUMA API to set memory policy
    struct bitmask* nodeMask = numa_allocate_nodemask();
    numa_bitmask_setbit(nodeMask, nodeId);

    int result = set_mempolicy(MPOL_PREFERRED, nodeMask->maskp, nodeMask->size + 1);
    numa_free_nodemask(nodeMask);

    if (result == 0)
    {
      return true;
    }
    else
    {
      return false;
    }
#else
    (void)nodeId;
    return false;
#endif
  }

  /**
   * @brief Get NUMA-aware core assignment for low-latency workloads
   * @param config Configuration for critical component assignment
   * @return CoreAssignment with NUMA locality considerations and isolated core optimization
   */
  static CoreAssignment getNumaAwareCoreAssignment(const CriticalComponentConfig& config = CriticalComponentConfig{})
  {
    CoreAssignment assignment;
    auto topology = getNumaTopology();

    if (!topology.numaAvailable || topology.nodes.empty())
    {
      // Fall back to regular assignment if NUMA not available
      return getRecommendedCoreAssignment(config);
    }

    auto isolatedCores = getIsolatedCores();
    assignment.hasIsolatedCores = !isolatedCores.empty();
    assignment.allIsolatedCores = isolatedCores;

    // If not using isolated cores or insufficient isolated cores, fall back
    if (!config.preferIsolatedCores ||
        (int)isolatedCores.size() < config.minIsolatedForCritical)
    {
      return getRecommendedCoreAssignment(config);
    }

    // Group isolated cores by NUMA node
    std::map<int, std::vector<int>> isolatedByNode;
    for (int coreId : isolatedCores)
    {
      int nodeId = getNumaNodeForCore(coreId);
      if (nodeId >= 0)
      {
        isolatedByNode[nodeId].push_back(coreId);
      }
    }

    // Sort components by priority for assignment
    std::vector<std::pair<std::string, int>> sortedComponents;
    for (const auto& [component, priority] : config.componentPriority)
    {
      sortedComponents.emplace_back(component, priority);
    }
    std::sort(sortedComponents.begin(), sortedComponents.end(),
              [](const auto& a, const auto& b)
              { return a.second < b.second; });

    // Try to assign all critical tasks to the same NUMA node first
    int bestNodeId = -1;
    size_t maxIsolatedCores = 0;

    for (const auto& [nodeId, cores] : isolatedByNode)
    {
      if (cores.size() > maxIsolatedCores)
      {
        maxIsolatedCores = cores.size();
        bestNodeId = nodeId;
      }
    }

    std::vector<int> availableIsolated;
    if (bestNodeId >= 0 && maxIsolatedCores >= sortedComponents.size())
    {
      // Use the best NUMA node for all critical tasks
      availableIsolated = isolatedByNode[bestNodeId];
    }
    else
    {
      // Distribute across multiple NUMA nodes
      availableIsolated = isolatedCores;
    }

    // Assign critical components to isolated cores
    std::vector<int>* targetCores = nullptr;

    for (const auto& [component, priority] : sortedComponents)
    {
      if (availableIsolated.empty() && !config.allowSharedCriticalCores)
      {
        break;
      }

      // Get target vector for this component
      if (component == "marketData")
        targetCores = &assignment.marketDataCores;
      else if (component == "execution")
        targetCores = &assignment.executionCores;
      else if (component == "strategy")
        targetCores = &assignment.strategyCores;
      else if (component == "risk")
        targetCores = &assignment.riskCores;
      else
        continue;

      if (targetCores && !availableIsolated.empty())
      {
        int coreId = availableIsolated.front();
        targetCores->push_back(coreId);
        assignment.criticalCores.push_back(coreId);

        int nodeId = getNumaNodeForCore(coreId);

        if (!config.allowSharedCriticalCores)
        {
          availableIsolated.erase(availableIsolated.begin());
        }
      }
    }

    // Handle remaining isolated cores
    if (!config.exclusiveIsolatedCores)
    {
      for (int core : availableIsolated)
      {
        assignment.generalCores.push_back(core);
      }
    }

    // Add non-isolated cores for general use, preferring same NUMA node as critical tasks
    std::set<int> usedCores(isolatedCores.begin(), isolatedCores.end());

    // First, add cores from the same NUMA node as critical tasks
    if (bestNodeId >= 0)
    {
      for (const auto& node : topology.nodes)
      {
        if (node.nodeId == bestNodeId)
        {
          for (int coreId : node.cpuCores)
          {
            if (usedCores.find(coreId) == usedCores.end())
            {
              assignment.generalCores.push_back(coreId);
            }
          }
          break;
        }
      }
    }

    // Then add cores from other NUMA nodes
    for (const auto& node : topology.nodes)
    {
      if (node.nodeId != bestNodeId)
      {
        for (int coreId : node.cpuCores)
        {
          if (usedCores.find(coreId) == usedCores.end())
          {
            assignment.generalCores.push_back(coreId);
          }
        }
      }
    }

    return assignment;
  }

  /**
   * @brief Convenience method to setup optimal isolated core configuration for low-latency systems
   * @param enableRealTimePriority Whether to set real-time priority for critical threads
   * @param disableFrequencyScaling Whether to disable CPU frequency scaling
   * @return CoreAssignment with optimal isolated core configuration
   */
  static CoreAssignment setupOptimalPerformanceConfiguration(bool enableRealTimePriority = true,
                                                             bool disableFrequencyScaling = true)
  {
    // Configure for maximum isolation and performance
    CriticalComponentConfig config;
    config.preferIsolatedCores = true;
    config.exclusiveIsolatedCores = true;
    config.allowSharedCriticalCores = false;
    config.minIsolatedForCritical = 1;

    // Get optimal core assignment
    auto assignment = getNumaAwareCoreAssignment(config);

    // Verify isolation
    bool isolated = verifyCriticalCoreIsolation(assignment);

    // Disable CPU frequency scaling for performance
    if (disableFrequencyScaling)
    {
      disableCpuFrequencyScaling();
    }

    return assignment;
  }

  /**
   * @brief Setup and pin all critical components using isolated cores
   * @param config Optional configuration for critical component assignment
   * @return true if all components successfully pinned to isolated cores
   */
  static bool setupAndPinCriticalComponents(const CriticalComponentConfig& config = CriticalComponentConfig{})
  {
    auto assignment = getNumaAwareCoreAssignment(config);

    std::vector<std::string> criticalComponents = {"marketData", "execution", "strategy", "risk"};
    std::vector<bool> pinnedSuccessfully(criticalComponents.size(), false);

    for (size_t i = 0; i < criticalComponents.size(); ++i)
    {
      pinnedSuccessfully[i] = pinCriticalComponent(criticalComponents[i], assignment);
    }

    // Check if all components were successfully pinned
    bool allPinned = std::all_of(pinnedSuccessfully.begin(), pinnedSuccessfully.end(),
                                 [](bool pinned)
                                 { return pinned; });

    return allPinned;
  }

  /**
   * @brief Check if system has sufficient isolated cores for low-latency workloads
   * @param minRequiredCores Minimum number of isolated cores required
   * @return true if system meets requirements
   */
  static bool checkIsolatedCoreRequirements(int minRequiredCores = 4)
  {
    auto isolatedCores = getIsolatedCores();
    int numCores = getNumCores();

    bool sufficient = (int)isolatedCores.size() >= minRequiredCores;

    return sufficient;
  }

  /**
   * @brief Comprehensive example demonstrating isolated core usage for low-latency applications
   */
  static void demonstrateIsolatedCoreUsage()
  {
    // Step 1: Check system capabilities
    bool hasRequiredCores = checkIsolatedCoreRequirements(4);

    // Step 2: Configure optimal settings
    CriticalComponentConfig config;
    config.preferIsolatedCores = true;
    config.exclusiveIsolatedCores = hasRequiredCores;     // Only exclusive if we have enough
    config.allowSharedCriticalCores = !hasRequiredCores;  // Allow sharing if limited cores
    config.minIsolatedForCritical = hasRequiredCores ? 4 : 1;

    // Customize priority based on strategy
    config.componentPriority["marketData"] = 0;  // Highest priority - market data is critical
    config.componentPriority["execution"] = 1;   // Second priority - order execution
    config.componentPriority["risk"] = 2;        // Third priority - risk management
    config.componentPriority["strategy"] = 3;    // Lowest priority - strategy computation

    auto assignment = getNumaAwareCoreAssignment(config);

    // Step 3: Verify isolation
    verifyCriticalCoreIsolation(assignment);

    // Step 4: Performance optimizations
    // Note: These would typically be done in separate threads for each component
  }

 private:
  static std::optional<std::string> readFile(const std::string& path)
  {
    std::ifstream file(path);
    if (!file.is_open())
    {
      return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
  }

  static bool writeFile(const std::string& path, const std::string& content)
  {
    std::ofstream file(path);
    if (!file.is_open())
    {
      return false;
    }

    file << content;
    return file.good();
  }

  static std::vector<int> parseIntList(const std::string& list)
  {
    std::vector<int> result;
    std::stringstream ss(list);
    std::string item;

    while (std::getline(ss, item, ','))
    {
      // Trim whitespace
      item.erase(item.find_last_not_of(" \n\r\t") + 1);
      item.erase(0, item.find_first_not_of(" \n\r\t"));

      if (item.empty())
        continue;

      size_t dashPos = item.find('-');
      if (dashPos != std::string::npos)
      {
        // Range like "2-5"
        int start = std::stoi(item.substr(0, dashPos));
        int end = std::stoi(item.substr(dashPos + 1));
        for (int i = start; i <= end; ++i)
        {
          result.push_back(i);
        }
      }
      else
      {
        // Single number
        result.push_back(std::stoi(item));
      }
    }

    return result;
  }

  static void parseNodeMemInfo(const std::string& memInfo, size_t& totalMemoryMB, size_t& freeMemoryMB)
  {
    std::stringstream ss(memInfo);
    std::string line;

    while (std::getline(ss, line))
    {
      if (line.find("MemTotal:") != std::string::npos)
      {
        size_t total = std::stoull(line.substr(line.find_first_of("0123456789") + 1));
        totalMemoryMB = total * 1024 / 1000;
      }
      else if (line.find("MemFree:") != std::string::npos)
      {
        size_t free = std::stoull(line.substr(line.find_first_of("0123456789") + 1));
        freeMemoryMB = free * 1024 / 1000;
      }
    }
  }
};

/**
 * @brief RAII wrapper for thread affinity management
 */
class ThreadAffinityGuard
{
 public:
  explicit ThreadAffinityGuard(int coreId)
  {
    _originalAffinity = CpuAffinity::getCurrentAffinity();
    CpuAffinity::pinToCore(coreId);
  }

  explicit ThreadAffinityGuard(const std::vector<int>& coreIds)
  {
    _originalAffinity = CpuAffinity::getCurrentAffinity();

#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    for (int coreId : coreIds)
    {
      CPU_SET(coreId, &cpuset);
    }

    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
#else
    (void)coreIds;
#endif
  }

  ~ThreadAffinityGuard()
  {
    if (_restored || _originalAffinity.empty())
    {
      return;
    }

#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    for (int coreId : _originalAffinity)
    {
      CPU_SET(coreId, &cpuset);
    }

    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
#endif

    _restored = true;
  }

  ThreadAffinityGuard(const ThreadAffinityGuard&) = delete;
  ThreadAffinityGuard& operator=(const ThreadAffinityGuard&) = delete;

 private:
  std::vector<int> _originalAffinity;
  bool _restored = false;
};

/**
 * @brief RAII wrapper for NUMA-aware thread affinity and memory policy management
 */
class NumaAffinityGuard
{
 public:
  explicit NumaAffinityGuard(int numaNodeId) : _numaNodeId(numaNodeId)
  {
    _originalAffinity = CpuAffinity::getCurrentAffinity();
    CpuAffinity::pinToNumaNode(numaNodeId);
    CpuAffinity::setMemoryPolicy(numaNodeId);
  }

  explicit NumaAffinityGuard(int coreId, int numaNodeId) : _numaNodeId(numaNodeId)
  {
    _originalAffinity = CpuAffinity::getCurrentAffinity();
    CpuAffinity::pinToCore(coreId);
    CpuAffinity::setMemoryPolicy(numaNodeId);
  }

  ~NumaAffinityGuard()
  {
    if (_restored || _originalAffinity.empty())
    {
      return;
    }

#ifdef __linux__
    // Restore original CPU affinity
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    for (int coreId : _originalAffinity)
    {
      CPU_SET(coreId, &cpuset);
    }

    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

#if FLOX_NUMA_AVAILABLE
    // Reset memory policy to default
    set_mempolicy(MPOL_DEFAULT, nullptr, 0);
#endif
#endif

    _restored = true;
  }

  NumaAffinityGuard(const NumaAffinityGuard&) = delete;
  NumaAffinityGuard& operator=(const NumaAffinityGuard&) = delete;

 private:
  std::vector<int> _originalAffinity;
  int _numaNodeId;
  bool _restored = false;
};

}  // namespace flox::performance
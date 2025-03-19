// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.

#ifndef KICAD_GIT_MEMORY_H
#define KICAD_GIT_MEMORY_H

#include <memory>
#include <git2.h>

namespace KIGIT
{
/**
* @brief A unique pointer for git_repository objects with automatic cleanup.
*/
using GitRepositoryPtr = std::unique_ptr<git_repository,
                                            decltype([](git_repository* aRepo) {
                                                git_repository_free(aRepo);
                                            })>;

/**
* @brief A unique pointer for git_reference objects with automatic cleanup.
*/
using GitReferencePtr = std::unique_ptr<git_reference,
                                           decltype([](git_reference* aRef) {
                                               git_reference_free(aRef);
                                           })>;

/**
* @brief A unique pointer for git_object objects with automatic cleanup.
*/
using GitObjectPtr = std::unique_ptr<git_object,
                                        decltype([](git_object* aObject) {
                                            git_object_free(aObject);
                                        })>;
/**
* @brief A unique pointer for git_commit objects with automatic cleanup.
*/
using GitCommitPtr = std::unique_ptr<git_commit,
                                        decltype([](git_commit* aCommit) {
                                            git_commit_free(aCommit);
                                        })>;

/**
* @brief A unique pointer for git_tree objects with automatic cleanup.
*/
using GitTreePtr = std::unique_ptr<git_tree,
                                      decltype([](git_tree* aTree) {
                                          git_tree_free(aTree);
                                      })>;

/**
* @brief A unique pointer for git_index objects with automatic cleanup.
*/
using GitIndexPtr = std::unique_ptr<git_index,
                                       decltype([](git_index* aIndex) {
                                           git_index_free(aIndex);
                                       })>;

/**
 * @brief A unique pointer for git_rebase objects with automatic cleanup.
 */
using GitRebasePtr = std::unique_ptr<git_rebase,
                                        decltype([](git_rebase* aRebase) {
                                            git_rebase_free(aRebase);
                                        })>;

/**
* @brief A unique pointer for git_revwalk objects with automatic cleanup.
*/
using GitRevWalkPtr = std::unique_ptr<git_revwalk,
                                         decltype([](git_revwalk* aWalker) {
                                             git_revwalk_free(aWalker);
                                         })>;

/**
* @brief A unique pointer for git_diff objects with automatic cleanup.
*/
using GitDiffPtr = std::unique_ptr<git_diff,
                                      decltype([](git_diff* aDiff) {
                                          git_diff_free(aDiff);
                                      })>;

/**
* @brief A unique pointer for git_signature objects with automatic cleanup.
*/
using GitSignaturePtr = std::unique_ptr<git_signature,
                                           decltype([](git_signature* aSignature) {
                                               git_signature_free(aSignature);
                                           })>;

/**
* @brief A unique pointer for git_config objects with automatic cleanup.
*/
using GitConfigPtr = std::unique_ptr<git_config,
                                        decltype([](git_config* aConfig) {
                                            git_config_free(aConfig);
                                        })>;

/**
* @brief A unique pointer for git_remote objects with automatic cleanup.
*/
using GitRemotePtr = std::unique_ptr<git_remote,
                                        decltype([](git_remote* aRemote) {
                                            git_remote_free(aRemote);
                                        })>;

/**
* @brief A unique pointer for git_annotated_commit objects with automatic cleanup.
*/
using GitAnnotatedCommitPtr = std::unique_ptr<git_annotated_commit,
                                                 decltype([](git_annotated_commit* aCommit) {
                                                     git_annotated_commit_free(aCommit);
                                                 })>;

/**
* @brief A unique pointer for git_oid objects with automatic cleanup.
*/
using GitOidPtr = std::unique_ptr<git_oid,
                                     decltype([](git_oid* aOid) {
                                         delete aOid;
                                     })>;

/**
* @brief A unique pointer for git_buf objects with automatic cleanup.
*/
using GitBufPtr = std::unique_ptr<git_buf,
                                     decltype([](git_buf* aBuf) {
                                         git_buf_free(aBuf);
                                     })>;

/**
* @brief A unique pointer for git_blame objects with automatic cleanup.
*/
using GitBlamePtr = std::unique_ptr<git_blame,
                                       decltype([](git_blame* aBlame) {
                                           git_blame_free(aBlame);
                                       })>;

/**
* @brief A unique pointer for git_blob objects with automatic cleanup.
*/
using GitBlobPtr = std::unique_ptr<git_blob,
                                      decltype([](git_blob* aBlob) {
                                          git_blob_free(aBlob);
                                      })>;

/**
* @brief A unique pointer for git_branch_iterator objects with automatic cleanup.
*/
using GitBranchIteratorPtr = std::unique_ptr<git_branch_iterator,
                                                decltype([](git_branch_iterator* aIter) {
                                                    git_branch_iterator_free(aIter);
                                                })>;

/**
* @brief A unique pointer for git_config_entry objects with automatic cleanup.
*/
using GitConfigEntryPtr = std::unique_ptr<git_config_entry,
                                             decltype([](git_config_entry* aEntry) {
                                                 git_config_entry_free(aEntry);
                                             })>;

/**
* @brief A unique pointer for git_config_iterator objects with automatic cleanup.
*/
using GitConfigIteratorPtr = std::unique_ptr<git_config_iterator,
                                                decltype([](git_config_iterator* aIter) {
                                                    git_config_iterator_free(aIter);
                                                })>;

/**
* @brief A unique pointer for git_credential objects with automatic cleanup.
*/
using GitCredentialPtr = std::unique_ptr<git_credential,
                                            decltype([](git_credential* aCred) {
                                                git_credential_free(aCred);
                                            })>;

/**
* @brief A unique pointer for git_oidarray objects with automatic cleanup.
*/
using GitOidArrayPtr = std::unique_ptr<git_oidarray,
                                          decltype([](git_oidarray* aArray) {
                                              git_oidarray_free(aArray);
                                          })>;

/**
* @brief A unique pointer for git_strarray objects with automatic cleanup.
*/
using GitStrArrayPtr = std::unique_ptr<git_strarray,
                                          decltype([](git_strarray* aArray) {
                                              git_strarray_free(aArray);
                                          })>;

/**
* @brief A unique pointer for git_describe_result objects with automatic cleanup.
*/
using GitDescribeResultPtr = std::unique_ptr<git_describe_result,
                                                decltype([](git_describe_result* aResult) {
                                                    git_describe_result_free(aResult);
                                                })>;

/**
* @brief A unique pointer for git_diff_stats objects with automatic cleanup.
*/
using GitDiffStatsPtr = std::unique_ptr<git_diff_stats,
                                           decltype([](git_diff_stats* aStats) {
                                               git_diff_stats_free(aStats);
                                           })>;

/**
* @brief A unique pointer for git_filter_list objects with automatic cleanup.
*/
using GitFilterListPtr = std::unique_ptr<git_filter_list,
                                            decltype([](git_filter_list* aFilters) {
                                                git_filter_list_free(aFilters);
                                            })>;

/**
* @brief A unique pointer for git_indexer objects with automatic cleanup.
*/
using GitIndexerPtr = std::unique_ptr<git_indexer,
                                         decltype([](git_indexer* aIdx) {
                                             git_indexer_free(aIdx);
                                         })>;

/**
* @brief A unique pointer for git_index_iterator objects with automatic cleanup.
*/
using GitIndexIteratorPtr = std::unique_ptr<git_index_iterator,
                                               decltype([](git_index_iterator* aIterator) {
                                                   git_index_iterator_free(aIterator);
                                               })>;

/**
* @brief A unique pointer for git_index_conflict_iterator objects with automatic cleanup.
*/
using GitIndexConflictIteratorPtr = std::unique_ptr<git_index_conflict_iterator,
                                                       decltype([](git_index_conflict_iterator* aIterator) {
                                                           git_index_conflict_iterator_free(aIterator);
                                                       })>;

/**
* @brief A unique pointer for git_status_list objects with automatic cleanup.
*/
using GitStatusListPtr = std::unique_ptr<git_status_list,
                                            decltype([](git_status_list* aList) {
                                                git_status_list_free(aList);
                                            })>;

} // namespace KIGIT

#endif  // KICAD_GIT_MEMORY_H

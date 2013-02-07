#region Using directives

using System;
using System.Collections.Generic;
using System.Text;
using SkmDataStructures2;

#endregion
// DataStructure from  4GuysFromRolla
namespace SkmDataStructures2
{
    /// <summary>
    /// Represents a node in a SkipList.  A SkipListNode has a Height and a set of neighboring
    /// SkipListNodes (precisely as many neighbor references as its Height, although some neighbor 
    /// references may be null).  Also, a SkipListNode contains some piece of data associated with it.
    /// </summary>
    /// <typeparam name="T">The type of the data stored in the SkipListNode.</typeparam>
    public class SkipListNode<T> : Node<T>
    {
        #region Constructors
        private SkipListNode() {}   // no default constructor available, must supply height
        public SkipListNode(int height)
        {
            base.Neighbors = new SkipListNodeList<T>(height);
        }

        public SkipListNode(T value, int height) : base(value)
        {
            base.Neighbors = new SkipListNodeList<T>(height);
        }
        #endregion

        #region Methods
        /// <summary>
        /// Increases the height of the SkipListNode by 1.
        /// </summary>
        internal void IncrementHeight()
        {
            // Increase height by 1
            ((SkipListNodeList<T>) base.Neighbors).IncrementHeight();
        }

        /// <summary>
        /// Decreases the height of the SkipListNode by 1.
        /// </summary>
        internal void DecrementHeight()
        {
            // Decrease height by 1
            ((SkipListNodeList<T>) base.Neighbors).DecrementHeight();
        }
        #endregion

        #region Public Properties
        /// <summary>
        /// Returns the height of the SkipListNode
        /// </summary>
        public int Height
        {
            get { return base.Neighbors.Count; }
        }

        /// <summary>
        /// Provides ordinally-indexed access to the neighbors of the SkipListNode.
        /// </summary>
        public SkipListNode<T> this[int index]
        {
            get { return (SkipListNode<T>) base.Neighbors[index]; }
            set { base.Neighbors[index] = value; }
        }
        #endregion
    }
}
